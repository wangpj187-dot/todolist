#include "GitClient.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <QByteArray>
#include <QTemporaryDir>

// ============================================================================
// Static Callback Functions
// ============================================================================

int GitClient::credentialsCallback(git_cred** out, const char* url,
                                   const char* username_from_url,
                                   unsigned int allowed_types,
                                   void* payload)
{
    Q_UNUSED(url);
    Q_UNUSED(username_from_url);

    GitClient* client = static_cast<GitClient*>(payload);
    if (!client) {
        return GIT_EUSER;
    }

    if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
        return git_cred_userpass_plaintext_new(
            out,
            kGitUsername,
            client->m_token.toUtf8().constData()
        );
    }

    return GIT_EUSER;
}

int GitClient::transferProgressCallback(const git_transfer_progress* stats,
                                         void* payload)
{
    GitClient* client = static_cast<GitClient*>(payload);
    if (!client || !stats) {
        return 0;
    }

    int progress = 0;
    if (stats->total_objects > 0) {
        progress = static_cast<int>((stats->received_objects * 100) / stats->total_objects);
    }

    QString message = QString("Receiving objects: %1/%2")
                          .arg(stats->received_objects)
                          .arg(stats->total_objects);

    emit client->syncProgress(progress, message);
    return 0;
}

int GitClient::validationCredentialsCallback(git_cred** out, const char* url,
                                              const char* username_from_url,
                                              unsigned int allowed_types,
                                              void* payload)
{
    Q_UNUSED(url);
    Q_UNUSED(username_from_url);

    ValidationPayload* credPayload = static_cast<ValidationPayload*>(payload);
    if (!credPayload) {
        return GIT_EUSER;
    }

    if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
        return git_cred_userpass_plaintext_new(
            out,
            kGitUsername,
            credPayload->token.toUtf8().constData()
        );
    }

    return GIT_EUSER;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

GitClient::GitClient(QObject* parent)
    : IGitClient(parent)
    , m_branch(kDefaultBranch)
    , m_repo(nullptr)
{
    git_libgit2_init();
    m_localPath = getDefaultLocalPath();
}

GitClient::~GitClient()
{
    closeRepository();
    git_libgit2_shutdown();
}

// ============================================================================
// Helpers
// ============================================================================

QString GitClient::getDefaultLocalPath() const
{
    return QDir::homePath() + "/" + kSyncDir;
}

QString GitClient::getLastError() const
{
    const git_error* e = giterr_last();
    if (e) {
        return QString::fromUtf8(e->message);
    }
    return QString("Unknown git error");
}

bool GitClient::ensureConfigured() const
{
    if (m_token.isEmpty() || m_repoUrl.isEmpty() || m_branch.isEmpty()) {
        emit const_cast<GitClient*>(this)->errorOccurred(
            "Git client is not configured. Call configure() first.");
        return false;
    }
    return true;
}

bool GitClient::ensureRepositoryOpen()
{
    if (!m_repo) {
        if (!openRepository()) {
            return false;
        }
    }
    return true;
}

void GitClient::closeRepository()
{
    if (m_repo) {
        git_repository_free(m_repo);
        m_repo = nullptr;
    }
}

bool GitClient::openRepository()
{
    closeRepository();

    if (m_localPath.isEmpty()) {
        emit errorOccurred("Local path is not set");
        return false;
    }

    QDir dir(m_localPath);
    if (!dir.exists()) {
        emit errorOccurred(QString("Repository path does not exist: %1").arg(m_localPath));
        return false;
    }

    int error = git_repository_open(&m_repo, m_localPath.toUtf8().constData());
    if (error != 0) {
        emit errorOccurred(QString("Failed to open repository: %1").arg(getLastError()));
        m_repo = nullptr;
        return false;
    }

    return true;
}

// ============================================================================
// Configuration
// ============================================================================

bool GitClient::configure(const QString& token, const QString& repoUrl, const QString& branch)
{
    if (token.isEmpty()) {
        emit errorOccurred("Token cannot be empty");
        return false;
    }
    if (repoUrl.isEmpty()) {
        emit errorOccurred("Repository URL cannot be empty");
        return false;
    }

    m_token = token;
    m_repoUrl = repoUrl;
    m_branch = branch.isEmpty() ? kDefaultBranch : branch;

    return true;
}

bool GitClient::isConfigured() const
{
    return !m_token.isEmpty() && !m_repoUrl.isEmpty() && !m_branch.isEmpty();
}

void GitClient::clearConfiguration()
{
    m_token.clear();
    m_repoUrl.clear();
    m_branch = kDefaultBranch;
    closeRepository();
}

// ============================================================================
// Repository Operations
// ============================================================================

bool GitClient::clone(const QString& localPath)
{
    if (!ensureConfigured()) {
        return false;
    }

    closeRepository();

    QString path = localPath.isEmpty() ? m_localPath : localPath;

    // Ensure parent directory exists
    QDir dir = QFileInfo(path).dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit errorOccurred(QString("Failed to create directory: %1").arg(dir.path()));
            return false;
        }
    }

    // Remove existing directory if it exists
    QDir targetDir(path);
    if (targetDir.exists()) {
        emit errorOccurred(QString("Directory already exists: %1").arg(path));
        return false;
    }

    git_clone_options cloneOpts = GIT_CLONE_OPTIONS_INIT;
    cloneOpts.fetch_opts.callbacks.credentials = credentialsCallback;
    cloneOpts.fetch_opts.callbacks.transfer_progress = transferProgressCallback;
    cloneOpts.fetch_opts.callbacks.payload = this;
    cloneOpts.checkout_branch = m_branch.toUtf8().constData();

    int error = git_clone(&m_repo,
                          m_repoUrl.toUtf8().constData(),
                          path.toUtf8().constData(),
                          &cloneOpts);

    if (error != 0) {
        emit errorOccurred(QString("Clone failed: %1").arg(getLastError()));
        if (m_repo) {
            git_repository_free(m_repo);
            m_repo = nullptr;
        }
        return false;
    }

    m_localPath = path;
    emit syncProgress(100, "Clone completed successfully");
    return true;
}

bool GitClient::init(const QString& localPath)
{
    closeRepository();

    QString path = localPath.isEmpty() ? m_localPath : localPath;

    // Ensure directory exists
    QDir dir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit errorOccurred(QString("Failed to create directory: %1").arg(path));
            return false;
        }
    }

    git_repository_init_options initOpts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    initOpts.flags = GIT_REPOSITORY_INIT_MKPATH;
    initOpts.initial_head = m_branch.toUtf8().constData();

    int error = git_repository_init_ext(&m_repo,
                                        path.toUtf8().constData(),
                                        &initOpts);

    if (error != 0) {
        emit errorOccurred(QString("Failed to initialize repository: %1").arg(getLastError()));
        m_repo = nullptr;
        return false;
    }

    m_localPath = path;
    return true;
}

bool GitClient::addRemote(const QString& remoteName, const QString& url)
{
    if (!ensureRepositoryOpen()) {
        return false;
    }

    QString name = remoteName.isEmpty() ? kDefaultRemote : remoteName;

    git_remote* remote = nullptr;
    int error = git_remote_create(&remote, m_repo,
                                  name.toUtf8().constData(),
                                  url.toUtf8().constData());

    if (error != 0) {
        emit errorOccurred(QString("Failed to add remote: %1").arg(getLastError()));
        if (remote) {
            git_remote_free(remote);
        }
        return false;
    }

    git_remote_free(remote);
    return true;
}

// ============================================================================
// Fetch / Pull / Push
// ============================================================================

bool GitClient::fetch()
{
    return fetchInternal(kDefaultRemote);
}

bool GitClient::fetchInternal(const QString& remoteName)
{
    if (!ensureConfigured() || !ensureRepositoryOpen()) {
        return false;
    }

    git_remote* remote = nullptr;
    int error = git_remote_lookup(&remote, m_repo, remoteName.toUtf8().constData());
    if (error != 0) {
        emit errorOccurred(QString("Remote not found: %1").arg(getLastError()));
        return false;
    }

    git_fetch_options fetchOpts = GIT_FETCH_OPTIONS_INIT;
    fetchOpts.callbacks.credentials = credentialsCallback;
    fetchOpts.callbacks.payload = this;
    fetchOpts.callbacks.transfer_progress = transferProgressCallback;

    error = git_remote_fetch(remote, nullptr, &fetchOpts, "fetch");
    git_remote_free(remote);

    if (error != 0) {
        emit errorOccurred(QString("Fetch failed: %1").arg(getLastError()));
        return false;
    }

    return true;
}

bool GitClient::pull()
{
    if (!ensureConfigured() || !ensureRepositoryOpen()) {
        return false;
    }

    emit syncProgress(0, "Starting pull...");

    // Step 1: Fetch
    if (!fetchInternal(kDefaultRemote)) {
        return false;
    }

    emit syncProgress(50, "Fetch completed, merging...");

    // Step 2: Merge
    if (!mergeInternal(kDefaultRemote, m_branch)) {
        return false;
    }

    emit syncProgress(100, "Pull completed successfully");
    return true;
}

bool GitClient::mergeInternal(const QString& remoteName, const QString& branchName)
{
    git_annotated_commit* remoteCommit = nullptr;
    if (!getRemoteBranchCommit(remoteName, branchName, &remoteCommit)) {
        return false;
    }

    // Get the reference to our HEAD
    git_reference* headRef = nullptr;
    int error = git_repository_head(&headRef, m_repo);
    if (error != 0) {
        emit errorOccurred(QString("Failed to get HEAD: %1").arg(getLastError()));
        git_annotated_commit_free(remoteCommit);
        return false;
    }

    // Get the OID of HEAD
    const git_oid* headOid = git_reference_target(headRef);
    if (!headOid) {
        emit errorOccurred("Failed to get HEAD OID");
        git_reference_free(headRef);
        git_annotated_commit_free(remoteCommit);
        return false;
    }

    // Perform merge analysis
    git_merge_analysis_t analysis;
    git_merge_preference_t preference;
    const git_annotated_commit* commits[] = { remoteCommit };

    error = git_merge_analysis(&analysis, &preference, m_repo, commits, 1);
    git_reference_free(headRef);

    if (error != 0) {
        emit errorOccurred(QString("Merge analysis failed: %1").arg(getLastError()));
        git_annotated_commit_free(remoteCommit);
        return false;
    }

    bool result = false;

    if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
        // Already up to date
        result = true;
    } else if (analysis & GIT_MERGE_ANALYSIS_FASTFORWARD) {
        // Fast-forward merge
        git_oid targetOid;
        git_oid_cpy(&targetOid, git_annotated_commit_id(remoteCommit));
        result = fastForwardMerge(&targetOid, remoteCommit);
    } else if (analysis & GIT_MERGE_ANALYSIS_NORMAL) {
        // Normal merge
        result = normalMerge(remoteCommit);
    } else {
        emit errorOccurred("Merge analysis returned an unsupported state");
        result = false;
    }

    git_annotated_commit_free(remoteCommit);

    // Check for conflicts after merge
    if (result && hasConflicts()) {
        QStringList conflicts = getConflictedFiles();
        for (const QString& file : conflicts) {
            emit conflictDetected(file);
        }
    }

    return result;
}

bool GitClient::fastForwardMerge(git_oid* targetOid, const git_annotated_commit* commit)
{
    Q_UNUSED(commit);

    // Get the current branch reference
    git_reference* branchRef = nullptr;
    int error = git_repository_head(&branchRef, m_repo);
    if (error != 0) {
        emit errorOccurred(QString("Failed to get HEAD reference: %1").arg(getLastError()));
        return false;
    }

    // Create a new reference pointing to the target commit
    git_reference* newRef = nullptr;
    error = git_reference_set_target(&newRef, branchRef, targetOid, "Fast-forward merge");
    git_reference_free(branchRef);

    if (error != 0) {
        emit errorOccurred(QString("Failed to update branch reference: %1").arg(getLastError()));
        return false;
    }

    git_reference_free(newRef);

    // Checkout the new commit to update the working directory
    git_checkout_options checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
    checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE;

    git_object* targetObj = nullptr;
    error = git_object_lookup(&targetObj, m_repo, targetOid, GIT_OBJECT_COMMIT);
    if (error != 0) {
        emit errorOccurred(QString("Failed to lookup target commit: %1").arg(getLastError()));
        return false;
    }

    error = git_checkout_tree(m_repo, targetObj, &checkoutOpts);
    git_object_free(targetObj);

    if (error != 0) {
        emit errorOccurred(QString("Checkout failed: %1").arg(getLastError()));
        return false;
    }

    return true;
}

bool GitClient::normalMerge(const git_annotated_commit* commit)
{
    git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
    git_checkout_options checkoutOpts = GIT_CHECKOUT_OPTIONS_INIT;
    checkoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE;

    const git_annotated_commit* commits[] = { commit };

    int error = git_merge(m_repo, commits, 1, &mergeOpts, &checkoutOpts);
    if (error != 0) {
        emit errorOccurred(QString("Merge failed: %1").arg(getLastError()));
        return false;
    }

    // If there are conflicts, return true but let the caller handle them
    if (git_repository_state(m_repo) == GIT_REPOSITORY_STATE_MERGE) {
        // Conflicts exist - return true but conflicts will be detected by hasConflicts()
        return true;
    }

    // No conflicts - create the merge commit
    git_oid treeOid;
    git_index* index = nullptr;
    error = git_repository_index(&index, m_repo);
    if (error != 0) {
        emit errorOccurred(QString("Failed to get index: %1").arg(getLastError()));
        return false;
    }

    if (git_index_has_conflicts(index)) {
        git_index_free(index);
        return true; // Conflicts exist, caller will handle
    }

    error = git_index_write_tree(&treeOid, index);
    git_index_free(index);

    if (error != 0) {
        emit errorOccurred(QString("Failed to write tree: %1").arg(getLastError()));
        return false;
    }

    // Get the two parent commits
    git_commit* headCommit = nullptr;
    if (!getHeadCommit(&headCommit)) {
        return false;
    }

    git_commit* remoteCommit = nullptr;
    error = git_commit_lookup(&remoteCommit, m_repo, git_annotated_commit_id(commit));
    if (error != 0) {
        emit errorOccurred(QString("Failed to lookup remote commit: %1").arg(getLastError()));
        git_commit_free(headCommit);
        return false;
    }

    const git_commit* parents[] = { headCommit, remoteCommit };

    git_oid commitOid;
    git_signature* sig = nullptr;
    error = git_signature_default(&sig, m_repo);
    if (error != 0) {
        // Create a default signature if none is configured
        error = git_signature_now(&sig, "TodoApp", "todoapp@local");
        if (error != 0) {
            emit errorOccurred(QString("Failed to create signature: %1").arg(getLastError()));
            git_commit_free(headCommit);
            git_commit_free(remoteCommit);
            return false;
        }
    }

    // Lookup the tree
    git_tree* tree = nullptr;
    error = git_tree_lookup(&tree, m_repo, &treeOid);
    if (error != 0) {
        emit errorOccurred(QString("Failed to lookup tree: %1").arg(getLastError()));
        git_signature_free(sig);
        git_commit_free(headCommit);
        git_commit_free(remoteCommit);
        return false;
    }

    error = git_commit_create(
        &commitOid,
        m_repo,
        "HEAD",
        sig,
        sig,
        nullptr,
        "Merge remote-tracking branch",
        tree,
        2,
        parents
    );

    git_tree_free(tree);
    git_signature_free(sig);
    git_commit_free(headCommit);
    git_commit_free(remoteCommit);

    if (error != 0) {
        emit errorOccurred(QString("Failed to create merge commit: %1").arg(getLastError()));
        return false;
    }

    // Clean up merge state
    git_repository_state_cleanup(m_repo);

    return true;
}

bool GitClient::push()
{
    if (!ensureConfigured() || !ensureRepositoryOpen()) {
        return false;
    }

    git_remote* remote = nullptr;
    int error = git_remote_lookup(&remote, m_repo, kDefaultRemote);
    if (error != 0) {
        emit errorOccurred(QString("Remote not found: %1").arg(getLastError()));
        return false;
    }

    // Set up push options
    git_push_options pushOpts = GIT_PUSH_OPTIONS_INIT;
    pushOpts.callbacks.credentials = credentialsCallback;
    pushOpts.callbacks.payload = this;

    // Create the refspec for pushing the current branch
    QString refspec = QString("refs/heads/%1:refs/heads/%1").arg(m_branch);

    char* refspecs[] = { const_cast<char*>(refspec.toUtf8().constData()) };
    git_strarray refspecArray = { refspecs, 1 };

    error = git_remote_push(remote, &refspecArray, &pushOpts);
    git_remote_free(remote);

    if (error != 0) {
        emit errorOccurred(QString("Push failed: %1").arg(getLastError()));
        return false;
    }

    return true;
}

IGitClient::SyncResult GitClient::sync()
{
    SyncResult result;
    result.success = false;
    result.changesPushed = 0;
    result.changesPulled = 0;
    result.conflicts = 0;
    result.completedAt = QDateTime::currentDateTimeUtc();

    if (!ensureConfigured()) {
        result.message = "Git client not configured";
        emit syncCompleted(result);
        return result;
    }

    if (!ensureRepositoryOpen()) {
        result.message = "Failed to open repository";
        emit syncCompleted(result);
        return result;
    }

    int initialAhead = 0;
    int initialBehind = 0;

    // Get initial status
    GitStatus initialStatus = getStatus();
    initialAhead = initialStatus.aheadCount;
    initialBehind = initialStatus.behindCount;

    emit syncProgress(0, "Starting sync...");

    // Step 1: Pull
    emit syncProgress(10, "Pulling latest changes...");
    if (!pull()) {
        result.message = "Pull failed during sync";
        emit syncCompleted(result);
        return result;
    }

    // Check for conflicts after pull
    if (hasConflicts()) {
        QStringList conflicts = getConflictedFiles();
        result.conflicts = conflicts.size();
        for (const QString& file : conflicts) {
            emit conflictDetected(file);
        }
        result.message = QString("Merge conflicts detected in %1 file(s)").arg(conflicts.size());
        emit syncCompleted(result);
        return result;
    }

    result.changesPulled = initialBehind;

    // Step 2: Commit local changes if any
    GitStatus afterPullStatus = getStatus();
    if (afterPullStatus.hasUncommittedChanges) {
        emit syncProgress(50, "Committing local changes...");
        if (!addFile(kBackupFileName)) {
            result.message = "Failed to stage backup file";
            emit syncCompleted(result);
            return result;
        }

        QString commitMsg = QString("Sync backup - %1")
                                .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        if (!commit(commitMsg)) {
            result.message = "Failed to commit local changes";
            emit syncCompleted(result);
            return result;
        }
    }

    // Step 3: Push
    GitStatus afterCommitStatus = getStatus();
    if (afterCommitStatus.aheadCount > 0) {
        emit syncProgress(80, "Pushing changes...");
        if (!push()) {
            result.message = "Push failed during sync";
            emit syncCompleted(result);
            return result;
        }
        result.changesPushed = afterCommitStatus.aheadCount;
    }

    emit syncProgress(100, "Sync completed successfully");

    result.success = true;
    result.message = "Sync completed successfully";
    result.completedAt = QDateTime::currentDateTimeUtc();
    emit syncCompleted(result);

    return result;
}

// ============================================================================
// File Operations
// ============================================================================

bool GitClient::addFile(const QString& filePath)
{
    if (!ensureRepositoryOpen()) {
        return false;
    }

    git_index* index = nullptr;
    int error = git_repository_index(&index, m_repo);
    if (error != 0) {
        emit errorOccurred(QString("Failed to get index: %1").arg(getLastError()));
        return false;
    }

    error = git_index_add_bypath(index, filePath.toUtf8().constData());
    if (error != 0) {
        emit errorOccurred(QString("Failed to add file to index: %1").arg(getLastError()));
        git_index_free(index);
        return false;
    }

    error = git_index_write(index);
    git_index_free(index);

    if (error != 0) {
        emit errorOccurred(QString("Failed to write index: %1").arg(getLastError()));
        return false;
    }

    return true;
}

bool GitClient::removeFile(const QString& filePath)
{
    if (!ensureRepositoryOpen()) {
        return false;
    }

    git_index* index = nullptr;
    int error = git_repository_index(&index, m_repo);
    if (error != 0) {
        emit errorOccurred(QString("Failed to get index: %1").arg(getLastError()));
        return false;
    }

    error = git_index_remove_bypath(index, filePath.toUtf8().constData());
    if (error != 0) {
        emit errorOccurred(QString("Failed to remove file from index: %1").arg(getLastError()));
        git_index_free(index);
        return false;
    }

    error = git_index_write(index);
    git_index_free(index);

    if (error != 0) {
        emit errorOccurred(QString("Failed to write index: %1").arg(getLastError()));
        return false;
    }

    // Also remove the file from the working directory
    QString fullPath = m_localPath + "/" + filePath;
    QFile file(fullPath);
    if (file.exists()) {
        file.remove();
    }

    return true;
}

bool GitClient::commit(const QString& message)
{
    if (!ensureRepositoryOpen()) {
        return false;
    }

    git_index* index = nullptr;
    int error = git_repository_index(&index, m_repo);
    if (error != 0) {
        emit errorOccurred(QString("Failed to get index: %1").arg(getLastError()));
        return false;
    }

    // Check if there are any changes to commit
    git_status_list* statusList = nullptr;
    git_status_options statusOpts = GIT_STATUS_OPTIONS_INIT;
    statusOpts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED;

    error = git_status_list_new(&statusList, m_repo, &statusOpts);
    if (error != 0) {
        emit errorOccurred(QString("Failed to get status: %1").arg(getLastError()));
        git_index_free(index);
        return false;
    }

    size_t statusCount = git_status_list_entrycount(statusList);
    git_status_list_free(statusList);

    if (statusCount == 0) {
        // Nothing to commit
        git_index_free(index);
        return true;
    }

    // Write the index to a tree
    git_oid treeOid;
    error = git_index_write_tree(&treeOid, index);
    git_index_free(index);

    if (error != 0) {
        emit errorOccurred(QString("Failed to write tree: %1").arg(getLastError()));
        return false;
    }

    // Get the parent commit (HEAD)
    git_oid parentOid;
    bool hasParent = false;
    git_reference* headRef = nullptr;

    error = git_repository_head(&headRef, m_repo);
    if (error == 0) {
        const git_oid* headOid = git_reference_target(headRef);
        if (headOid) {
            git_oid_cpy(&parentOid, headOid);
            hasParent = true;
        }
        git_reference_free(headRef);
    }

    // Create the commit
    git_oid commitOid;
    git_signature* sig = nullptr;

    error = git_signature_default(&sig, m_repo);
    if (error != 0) {
        error = git_signature_now(&sig, "TodoApp", "todoapp@local");
        if (error != 0) {
            emit errorOccurred(QString("Failed to create signature: %1").arg(getLastError()));
            return false;
        }
    }

    git_tree* tree = nullptr;
    error = git_tree_lookup(&tree, m_repo, &treeOid);
    if (error != 0) {
        emit errorOccurred(QString("Failed to lookup tree: %1").arg(getLastError()));
        git_signature_free(sig);
        return false;
    }

    const git_commit* parents[1];
    git_commit* parentCommit = nullptr;
    int parentCount = 0;

    if (hasParent) {
        error = git_commit_lookup(&parentCommit, m_repo, &parentOid);
        if (error == 0) {
            parents[0] = parentCommit;
            parentCount = 1;
        }
    }

    error = git_commit_create(
        &commitOid,
        m_repo,
        "HEAD",
        sig,
        sig,
        nullptr,
        message.toUtf8().constData(),
        tree,
        parentCount,
        parentCount > 0 ? parents : nullptr
    );

    git_tree_free(tree);
    git_signature_free(sig);
    if (parentCommit) {
        git_commit_free(parentCommit);
    }

    if (error != 0) {
        emit errorOccurred(QString("Failed to create commit: %1").arg(getLastError()));
        return false;
    }

    return true;
}

// ============================================================================
// Status
// ============================================================================

IGitClient::GitStatus GitClient::getStatus()
{
    GitStatus status;
    status.hasUncommittedChanges = false;
    status.aheadCount = 0;
    status.behindCount = 0;
    status.currentBranch = m_branch;

    if (!ensureRepositoryOpen()) {
        return status;
    }

    // Get current branch
    git_reference* headRef = nullptr;
    int error = git_repository_head(&headRef, m_repo);
    if (error == 0) {
        const char* branchName = git_reference_shorthand(headRef);
        if (branchName) {
            status.currentBranch = QString::fromUtf8(branchName);
        }
        git_reference_free(headRef);
    }

    // Check for uncommitted changes
    git_status_list* statusList = nullptr;
    git_status_options statusOpts = GIT_STATUS_OPTIONS_INIT;
    statusOpts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                       GIT_STATUS_OPT_EXCLUDE_SUBMODULES;

    error = git_status_list_new(&statusList, m_repo, &statusOpts);
    if (error == 0) {
        size_t count = git_status_list_entrycount(statusList);
        status.hasUncommittedChanges = count > 0;
        git_status_list_free(statusList);
    }

    // Calculate ahead/behind counts relative to upstream
    git_reference* localBranch = nullptr;
    error = git_repository_head(&localBranch, m_repo);
    if (error == 0) {
        if (git_reference_is_branch(localBranch) == 1) {
            git_reference* upstream = nullptr;
            error = git_branch_upstream(&upstream, localBranch);
            if (error == 0) {
                const git_oid* localOid = git_reference_target(localBranch);
                const git_oid* upstreamOid = git_reference_target(upstream);

                if (localOid && upstreamOid) {
                    size_t ahead = 0, behind = 0;
                    error = git_graph_ahead_behind(&ahead, &behind, m_repo, localOid, upstreamOid);
                    if (error == 0) {
                        status.aheadCount = static_cast<int>(ahead);
                        status.behindCount = static_cast<int>(behind);
                    }
                }
                git_reference_free(upstream);
            }
        }
        git_reference_free(localBranch);
    }

    return status;
}

bool GitClient::hasConflicts()
{
    if (!ensureRepositoryOpen()) {
        return false;
    }

    git_index* index = nullptr;
    int error = git_repository_index(&index, m_repo);
    if (error != 0) {
        return false;
    }

    bool hasConflicts = git_index_has_conflicts(index) != 0;
    git_index_free(index);

    return hasConflicts;
}

QStringList GitClient::getConflictedFiles()
{
    QStringList files;

    if (!ensureRepositoryOpen()) {
        return files;
    }

    git_index* index = nullptr;
    int error = git_repository_index(&index, m_repo);
    if (error != 0) {
        return files;
    }

    git_index_conflict_iterator* iter = nullptr;
    error = git_index_conflict_iterator_new(&iter, index);
    if (error != 0) {
        git_index_free(index);
        return files;
    }

    const git_index_entry* ancestor;
    const git_index_entry* our;
    const git_index_entry* their;

    while (git_index_conflict_next(&ancestor, &our, &their, iter) == 0) {
        if (our) {
            files.append(QString::fromUtf8(our->path));
        } else if (their) {
            files.append(QString::fromUtf8(their->path));
        } else if (ancestor) {
            files.append(QString::fromUtf8(ancestor->path));
        }
    }

    git_index_conflict_iterator_free(iter);
    git_index_free(index);

    return files;
}

// ============================================================================
// Conflict Resolution
// ============================================================================

bool GitClient::resolveConflict(const QString& filePath, bool useLocal)
{
    if (!ensureRepositoryOpen()) {
        return false;
    }

    git_index* index = nullptr;
    int error = git_repository_index(&index, m_repo);
    if (error != 0) {
        emit errorOccurred(QString("Failed to get index: %1").arg(getLastError()));
        return false;
    }

    // Get the conflict entries
    const git_index_entry* ancestor;
    const git_index_entry* our;
    const git_index_entry* their;

    error = git_index_conflict_get(&ancestor, &our, &their, index,
                                   filePath.toUtf8().constData());
    if (error != 0) {
        emit errorOccurred(QString("Conflict not found for file: %1").arg(filePath));
        git_index_free(index);
        return false;
    }

    // Choose which version to keep
    const git_index_entry* chosen = useLocal ? our : their;
    if (!chosen) {
        // If the chosen side is null (file was deleted), remove the file
        if (useLocal && !our) {
            // Local was deleted - remove from index
            git_index_remove_bypath(index, filePath.toUtf8().constData());
        } else if (!useLocal && !their) {
            // Remote was deleted - remove from index
            git_index_remove_bypath(index, filePath.toUtf8().constData());
        } else {
            emit errorOccurred("Chosen side of conflict is null");
            git_index_free(index);
            return false;
        }
    } else {
        // Add the chosen version to the index
        error = git_index_add_frombuffer(index, chosen, nullptr, 0);
        if (error != 0) {
            // Try the simpler approach - add by path after writing the file
            QString fullPath = m_localPath + "/" + filePath;
            QString content = getFileContent(filePath, useLocal);

            if (content.isEmpty()) {
                emit errorOccurred(QString("Failed to get file content for: %1").arg(filePath));
                git_index_free(index);
                return false;
            }

            QFile file(fullPath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                emit errorOccurred(QString("Failed to open file for writing: %1").arg(filePath));
                git_index_free(index);
                return false;
            }

            file.write(content.toUtf8());
            file.close();

            error = git_index_add_bypath(index, filePath.toUtf8().constData());
            if (error != 0) {
                emit errorOccurred(QString("Failed to add resolved file: %1").arg(getLastError()));
                git_index_free(index);
                return false;
            }
        }
    }

    // Remove the conflict
    error = git_index_conflict_remove(index, filePath.toUtf8().constData());
    if (error != 0) {
        emit errorOccurred(QString("Failed to remove conflict: %1").arg(getLastError()));
        git_index_free(index);
        return false;
    }

    error = git_index_write(index);
    git_index_free(index);

    if (error != 0) {
        emit errorOccurred(QString("Failed to write index: %1").arg(getLastError()));
        return false;
    }

    return true;
}

QString GitClient::getFileContent(const QString& filePath, bool localVersion)
{
    if (!ensureRepositoryOpen()) {
        return QString();
    }

    if (localVersion) {
        return readFileFromWorkdir(filePath);
    }

    // Get remote version from the upstream branch
    git_commit* remoteCommit = nullptr;
    git_reference* localBranch = nullptr;
    int error = git_repository_head(&localBranch, m_repo);
    if (error != 0) {
        emit errorOccurred(QString("Failed to get HEAD: %1").arg(getLastError()));
        return QString();
    }

    git_reference* upstream = nullptr;
    error = git_branch_upstream(&upstream, localBranch);
    git_reference_free(localBranch);

    if (error != 0) {
        emit errorOccurred(QString("Failed to get upstream branch: %1").arg(getLastError()));
        return QString();
    }

    const git_oid* upstreamOid = git_reference_target(upstream);
    if (!upstreamOid) {
        git_reference_free(upstream);
        return QString();
    }

    error = git_commit_lookup(&remoteCommit, m_repo, upstreamOid);
    git_reference_free(upstream);

    if (error != 0) {
        emit errorOccurred(QString("Failed to lookup upstream commit: %1").arg(getLastError()));
        return QString();
    }

    QString content = readFileFromCommit(filePath, remoteCommit);
    git_commit_free(remoteCommit);

    return content;
}

QString GitClient::readFileFromWorkdir(const QString& filePath) const
{
    QString fullPath = m_localPath + "/" + filePath;
    QFile file(fullPath);

    if (!file.exists()) {
        return QString();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QByteArray content = file.readAll();
    file.close();

    return QString::fromUtf8(content);
}

QString GitClient::readFileFromCommit(const QString& filePath, git_commit* commit) const
{
    if (!commit) {
        return QString();
    }

    git_tree* tree = nullptr;
    int error = git_commit_tree(&tree, commit);
    if (error != 0) {
        return QString();
    }

    git_tree_entry* entry = nullptr;
    error = git_tree_entry_bypath(&entry, tree, filePath.toUtf8().constData());
    git_tree_free(tree);

    if (error != 0) {
        return QString();
    }

    git_object* obj = nullptr;
    error = git_tree_entry_to_object(&obj, m_repo, entry);
    git_tree_entry_free(entry);

    if (error != 0) {
        return QString();
    }

    if (git_object_type(obj) != GIT_OBJECT_BLOB) {
        git_object_free(obj);
        return QString();
    }

    git_blob* blob = reinterpret_cast<git_blob*>(obj);
    QString content = QString::fromUtf8(
        static_cast<const char*>(git_blob_rawcontent(blob)),
        static_cast<int>(git_blob_rawsize(blob))
    );

    git_blob_free(blob);

    return content;
}

// ============================================================================
// History
// ============================================================================

bool GitClient::getHeadCommit(git_commit** outCommit) const
{
    git_reference* headRef = nullptr;
    int error = git_repository_head(&headRef, m_repo);
    if (error != 0) {
        emit const_cast<GitClient*>(this)->errorOccurred(
            QString("Failed to get HEAD: %1").arg(getLastError()));
        return false;
    }

    const git_oid* headOid = git_reference_target(headRef);
    if (!headOid) {
        git_reference_free(headRef);
        return false;
    }

    error = git_commit_lookup(outCommit, m_repo, headOid);
    git_reference_free(headRef);

    if (error != 0) {
        emit const_cast<GitClient*>(this)->errorOccurred(
            QString("Failed to lookup HEAD commit: %1").arg(getLastError()));
        return false;
    }

    return true;
}

bool GitClient::getRemoteBranchCommit(const QString& remoteName, const QString& branchName,
                                      git_annotated_commit** outCommit)
{
    QString refName = QString("refs/remotes/%1/%2").arg(remoteName, branchName);

    git_reference* remoteRef = nullptr;
    int error = git_reference_lookup(&remoteRef, m_repo, refName.toUtf8().constData());
    if (error != 0) {
        emit errorOccurred(QString("Remote branch not found: %1").arg(getLastError()));
        return false;
    }

    const git_oid* remoteOid = git_reference_target(remoteRef);
    if (!remoteOid) {
        git_reference_free(remoteRef);
        emit errorOccurred("Failed to get remote branch OID");
        return false;
    }

    error = git_annotated_commit_lookup(outCommit, m_repo, remoteOid);
    git_reference_free(remoteRef);

    if (error != 0) {
        emit errorOccurred(QString("Failed to lookup remote commit: %1").arg(getLastError()));
        return false;
    }

    return true;
}

QString GitClient::getLastCommitHash()
{
    if (!ensureRepositoryOpen()) {
        return QString();
    }

    git_commit* commit = nullptr;
    if (!getHeadCommit(&commit)) {
        return QString();
    }

    const git_oid* oid = git_commit_id(commit);
    char hash[GIT_OID_HEXSZ + 1];
    git_oid_fmt(hash, oid);
    hash[GIT_OID_HEXSZ] = '\0';

    git_commit_free(commit);

    return QString::fromUtf8(hash);
}

QDateTime GitClient::getLastCommitTime()
{
    if (!ensureRepositoryOpen()) {
        return QDateTime();
    }

    git_commit* commit = nullptr;
    if (!getHeadCommit(&commit)) {
        return QDateTime();
    }

    git_time_t commitTime = git_commit_time(commit);
    git_commit_free(commit);

    return QDateTime::fromSecsSinceEpoch(commitTime, Qt::UTC);
}

// ============================================================================
// Authentication / Connection Testing
// ============================================================================

bool GitClient::testConnection()
{
    if (!ensureConfigured()) {
        return false;
    }

    // Try to fetch from the remote to test connection
    if (!ensureRepositoryOpen()) {
        // If repo doesn't exist, try a quick ls-remote test
        return validateToken(m_token, m_repoUrl);
    }

    return fetchInternal(kDefaultRemote);
}

bool GitClient::validateToken(const QString& token, const QString& repoUrl)
{
    if (token.isEmpty() || repoUrl.isEmpty()) {
        return false;
    }

    // Create a temporary repo for testing
    git_repository* tempRepo = nullptr;
    git_remote* remote = nullptr;
    bool success = false;

    // Create a temporary directory
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        return false;
    }

    QString tempPath = tempDir.path();

    // Initialize a bare repo for ls-remote
    int error = git_repository_init(&tempRepo, tempPath.toUtf8().constData(), 1);
    if (error != 0) {
        return false;
    }

    // Create a remote
    error = git_remote_create(&remote, tempRepo, "origin", repoUrl.toUtf8().constData());
    if (error != 0) {
        git_repository_free(tempRepo);
        return false;
    }

    // Set up connect options with credentials
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    callbacks.credentials = validationCredentialsCallback;

    ValidationPayload payload;
    payload.token = token;
    callbacks.payload = &payload;

    error = git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks, nullptr, nullptr);
    if (error == 0) {
        // Connection succeeded - try to list references
        const git_remote_head** refs = nullptr;
        size_t refsCount = 0;
        error = git_remote_ls(&refs, &refsCount, remote);
        if (error == 0) {
            success = true;
        }
    }

    git_remote_free(remote);
    git_repository_free(tempRepo);

    return success;
}
