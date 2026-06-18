# Todolist Application Tasks

## Phase 1: Foundation (Completed)
- [x] T001: Project setup with CMake and Qt 6.5+
- [x] T002: SQLite database schema and initialization
- [x] T003: Data models (Todo, Category, Config)
- [x] T004: DatabaseManager with CRUD operations
- [x] T005: TodoListModel (QAbstractListModel)
- [x] T006: JsonSerializer for backup format
- [x] T007: Main QML UI with basic list view
- [x] T008: TodoForm for add/edit/delete

## Phase 2: Core Features (Completed)
- [x] T009: Priority system (Low/Medium/High/Urgent)
- [x] T010: Category management
- [x] T011: Due date support
- [x] T012: Filtering (status, priority, category)
- [x] T013: Search functionality
- [x] T014: Sorting options
- [x] T015: TodoService business logic
- [x] T016: QML-C++ integration

## Phase 3: UI Polish (Completed)
- [x] T003-FIX: Rewrite model headers with QObject/Q_PROPERTY
- [x] T005-FIX: Add filterPriority property
- [x] T015-FIX: Status filter "All" support
- [x] T016-FIX: Enum scoping across codebase

## Phase 4: GitHub Sync (Completed)
- [x] T017: GitClient with libgit2 (clone/pull/push/commit)
- [x] T018: SyncService with conflict detection
- [x] T019: GitHubApiClient for token validation
- [x] T020: SyncStatusPanel QML component
- [x] T021: GitHubSettings QML component
- [x] T022: Auto-sync timer
- [x] T023: ConflictResolutionDialog QML component

## Phase 5: Desktop Widget (Completed)
- [x] T024: WidgetWindow.qml with frameless always-on-top flags
- [x] T025: Window position/size persistence via ConfigService
- [x] T026: WidgetView.qml compact view for high-priority todos
- [x] T027: SystemTrayManager with QSystemTrayIcon
- [x] T028: Tray menu with show/hide, add todo, sync, settings, quit

## Phase 6: Supporting Services (Completed - Stub Implementations)
- [x] ThemeService stub implementation
- [x] ReminderService stub implementation
- [x] StatsService stub implementation

## Phase 6: Categories + Tags (Pending)
- [ ] T029: Category CRUD UI
- [ ] T030: Category color customization
- [ ] T031: Tag system
- [ ] T032: Tag management UI
- [ ] T033: Filter by multiple tags
- [ ] T034: Category statistics
- [ ] T035: Tag autocomplete

## Phase 7: Reminders + Tray (Pending)
- [ ] T036: ReminderService implementation
- [ ] T037: Due date reminders
- [ ] T038: Snooze/dismiss functionality
- [ ] T039: Tray notifications
- [ ] T040: Recurring todos
- [ ] T041: Reminder sound

## Phase 8: Statistics (Pending)
- [ ] T042: StatsService implementation
- [ ] T043: Statistics dashboard UI
- [ ] T044: Completion rate charts
- [ ] T045: Category distribution
- [ ] T046: Daily/weekly trends
- [ ] T047: Export statistics

## Phase 9: Themes (Pending)
- [ ] T048: ThemeService implementation
- [ ] T049: Light/dark/system theme switching
- [ ] T050: Custom theme editor
- [ ] T051: Accent color customization
- [ ] T052: Theme import/export
- [ ] T053: System theme detection

## Phase 10: Animations (Pending)
- [ ] T054: List item animations
- [ ] T055: Page transitions
- [ ] T056: Loading states
- [ ] T057: Micro-interactions

## Phase 11: Polish & Cross-Cutting (Pending)
- [ ] T058: Error handling & logging
- [ ] T059: Input validation
- [ ] T060: Accessibility
- [ ] T061: Keyboard shortcuts
- [ ] T062: Auto-updates
- [ ] T063: Documentation
- [ ] T064: Performance optimizations
- [ ] T065: Cross-platform testing
- [ ] T066: Final build & packaging
