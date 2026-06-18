---
After all tasks are completed, the following test skills will be automatically applied to validate code quality:

- **flux-consistency-check** – Check consistency between generated code and spec documents (spec.md, plan.md, tasks.md)
- **bits-unit-test-gen** – Unit tests to verify correctness of individual components
- **bam** – BAM API management and IDL consistency validation to ensure API contracts are met
- **bits-code-guard** – Automated code review for style, best practices, and maintainability
- **flux-code-style** – Multi-language code style and standards review
---

# Tasks: Todolist 桌面摆件应用

**Input**: Design documents from `flux/changes/20260618_todolist-app/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**⚠️ NO UNIT TESTS**: Do NOT generate any unit test, mock, or test-related tasks. Unit test generation is handled exclusively by the implement phase (skill `flux-implement`). This applies to all phases including Polish & Cross-Cutting Concerns.

**Organization**: Tasks are grouped by user story to enable independent implementation of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **Single project**: `src/`, `tests/` at repository root
- Paths shown below assume single project structure per plan.md

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure

- [x] T001 Create project directory structure (src/models, src/business, src/data, src/qml/components, src/qml/themes, assets, tests) at repos/devinfra/boe_inspiration_wiki_wangpengjun_187/
- [x] T002 [P] Create CMakeLists.txt with Qt 6.5+, libgit2, nlohmann/json dependencies in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/CMakeLists.txt
- [x] T003 [P] Create .gitignore file for C++/Qt project in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/.gitignore
- [x] T004 [P] Create main.cpp with QApplication and QQmlApplicationEngine setup in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/main.cpp

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [x] T005 [P] Create data model headers (Todo.h, Category.h, Config.h) with enums (Priority, TodoStatus, SyncStatus) and properties in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/models/
- [x] T006 [P] Create DatabaseManager with SQLite initialization, schema migration (v1), and base CRUD operations in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/data/DatabaseManager.h and .cpp
- [x] T007 [P] Create JsonSerializer for Todo/Category/Config JSON serialization using nlohmann/json in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/data/JsonSerializer.h and .cpp
- [x] T008 [P] Create all interface headers (ITodoService, ISyncService, IReminderService, IStatsService, IThemeService, IConfigService, IDatabaseManager, IJsonSerializer, IGitClient) in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/business/interfaces/
- [x] T009 [P] Create encryption utility for secure token storage (AES-256-GCM + platform-specific key management) in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/data/EncryptionUtil.h and .cpp

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - 待办事项 CRUD + 优先级管理 (Priority: P0) 🎯 MVP

**Goal**: Implement core todo management with create, view, edit, delete, mark complete, and 4-level priority support

**Acceptance Criteria**:
- User can create a new todo with title, description, priority, category, and due date
- User can view all todos in a list sorted by priority and creation date
- User can edit existing todo details
- User can delete a todo with confirmation
- User can mark a todo as complete/incomplete/cancelled
- User can filter todos by status and priority
- All changes persist to SQLite database
- Application starts and displays correctly on Windows and macOS

### Implementation for User Story 1

- [ ] T010 [P] [US1] Implement TodoService (ITodoService) with full CRUD, filtering, sorting, and category/tag operations in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/business/TodoService.h and .cpp
- [ ] T011 [P] [US1] Create TodoListModel QAbstractListModel for QML data binding in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/models/TodoListModel.h and .cpp
- [ ] T012 [P] [US1] Create main.qml with basic window layout and todo list view in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/main.qml
- [ ] T013 [P] [US1] Create TodoItem.qml component for individual todo display with checkbox, title, priority indicator in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/TodoItem.qml
- [ ] T014 [P] [US1] Create TodoForm.qml for create/edit todo dialog with title input, description, priority selector, due date picker in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/TodoForm.qml
- [ ] T015 [US1] Register services and models to QML context properties in main.cpp (per qml-cpp-bridge.md contract)
- [ ] T016 [US1] Implement priority filtering and status sorting controls in main.qml UI

**Checkpoint**: At this point, User Story 1 should be fully functional - core todo management works end-to-end

---

## Phase 4: User Story 2 - GitHub 云端备份同步 (Priority: P0)

**Goal**: Implement GitHub repository backup with automatic/manual sync, conflict detection, and resolution

**Acceptance Criteria**:
- User can configure GitHub Token, repo address, and branch in settings
- User can test GitHub connection to verify configuration
- User can manually trigger sync (push/pull)
- Application automatically syncs at configured interval (default 30 min)
- Conflicts are detected and presented to user for resolution (use local/use remote)
- Sync status and last sync time are displayed in UI
- Token is encrypted before storage using AES-256-GCM
- Backup data is in JSON format per data-model.md specification

### Implementation for User Story 2

- [ ] T017 [P] [US2] Implement GitClient (IGitClient) using libgit2 for clone/pull/push/commit operations in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/data/GitClient.h and .cpp
- [ ] T018 [P] [US2] Implement SyncService (ISyncService) with sync orchestration, conflict detection/resolution logic in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/business/SyncService.h and .cpp
- [ ] T019 [P] [US2] Create GitHubApiClient for token validation and repo info using QNetworkAccessManager in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/data/GitHubApiClient.h and .cpp
- [ ] T020 [P] [US2] Create SyncStatusPanel.qml for sync status display, progress indicator, and manual sync button in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/SyncStatusPanel.qml
- [ ] T021 [P] [US2] Create GitHubSettings.qml for token input, repo address, branch configuration, and connection test in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/GitHubSettings.qml
- [ ] T022 [US2] Implement automatic sync timer in SyncService based on ConfigService settings
- [ ] T023 [US2] Create ConflictResolutionDialog.qml showing local vs remote todo differences with resolution options in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/ConflictResolutionDialog.qml

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently

---

## Phase 5: User Story 3 - 桌面摆件特性 (Priority: P0)

**Goal**: Implement desktop widget features: frameless window, always-on-top, draggable, opacity control

**Acceptance Criteria**:
- Window has no title bar (frameless)
- Window stays on top of other applications by default
- User can drag the window by clicking and dragging on empty areas
- User can adjust window opacity (0.5 - 1.0) via settings slider
- Window position and size are saved and restored on restart
- User can minimize to system tray
- Window supports resize from corners

### Implementation for User Story 3

- [ ] T024 [P] [US3] Implement frameless window with Qt.Window flags and custom drag handling in main.qml
- [ ] T025 [P] [US3] Implement always-on-top window flag with toggle binding to ConfigService in main.qml
- [ ] T026 [P] [US3] Create WindowControls.qml with minimize, close, and settings buttons in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/WindowControls.qml
- [ ] T027 [P] [US3] Implement window opacity control with slider binding to config in main.qml
- [ ] T028 [US3] Implement window position (x, y) and size (width, height) persistence via ConfigService in main.cpp

**Checkpoint**: At this point, User Stories 1, 2, AND 3 should all work independently

---

## Phase 6: User Story 4 - 分类标签管理 (Priority: P1)

**Goal**: Implement category and tag management with color customization and filtering

**Acceptance Criteria**:
- User can create/edit/delete categories with custom name, color, and icon
- User can add/remove tags to individual todos
- User can filter todos by category
- Tags are displayed as colored chips on todo items
- Category colors are used for visual distinction in the UI
- Deleting a category does not delete associated todos (sets category to null)

### Implementation for User Story 4

- [ ] T029 [P] [US4] Extend TodoService with category CRUD operations (create/update/delete) in TodoService.cpp
- [ ] T030 [P] [US4] Extend TodoService with tag management (add/remove tags to todos, get all tags) in TodoService.cpp
- [ ] T031 [P] [US4] Create CategoryList.qml component for category management (add/edit/delete with color picker) in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/CategoryList.qml
- [ ] T032 [P] [US4] Create TagChip.qml component for tag display with color background and remove button in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/TagChip.qml
- [ ] T033 [P] [US4] Add category filter dropdown to main todo list view in main.qml
- [ ] T034 [US4] Add color picker component for category/tag color customization in ColorPicker.qml
- [ ] T035 [US4] Update TodoItem.qml to display category color indicator and tag chips

**Checkpoint**: At this point, User Stories 1-4 should all work independently

---

## Phase 7: User Story 5 - 截止日期提醒 + 系统托盘 (Priority: P1)

**Goal**: Implement due date reminders with system tray notifications and popup alerts

**Acceptance Criteria**:
- User receives notification when a todo is approaching its due date (configurable advance time)
- System tray icon shows application status
- User can access menu from system tray (show window, quick add, settings, quit)
- User can dismiss or snooze reminders
- User can enable/disable reminders globally
- User can configure reminder advance time (default 60 minutes)
- Notifications work on both Windows (toast notifications) and macOS (Notification Center)

### Implementation for User Story 5

- [ ] T036 [P] [US5] Implement ReminderService (IReminderService) with QTimer for periodic due date checking in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/business/ReminderService.h and .cpp
- [ ] T037 [P] [US5] Create SystemTray integration with QSystemTrayIcon (context menu, notification display) in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/business/SystemTrayManager.h and .cpp
- [ ] T038 [P] [US5] Create ReminderNotification.qml for popup reminder display with dismiss/snooze buttons in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/ReminderNotification.qml
- [ ] T039 [P] [US5] Create ReminderSettings.qml for enable toggle and advance time configuration in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/ReminderSettings.qml
- [ ] T040 [US5] Add due date picker to TodoForm.qml with calendar popup
- [ ] T041 [US5] Register SystemTrayManager and connect to ReminderService signals in main.cpp

**Checkpoint**: At this point, User Stories 1-5 should all work independently

---

## Phase 8: User Story 6 - 数据统计可视化 (Priority: P1)

**Goal**: Implement data statistics with completion rate, category distribution, and daily trend charts

**Acceptance Criteria**:
- User can view total todos count, completed count, and completion rate percentage
- User can view pending and overdue todo counts
- User can view category distribution as pie chart or bar chart
- User can view daily completion trend for last 7 days as line chart
- Statistics update automatically when todos change
- Charts use Qt Charts module for rendering
- User can refresh statistics manually

### Implementation for User Story 6

- [ ] T042 [P] [US6] Implement StatsService (IStatsService) with completion rate, category stats, trend calculations in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/business/StatsService.h and .cpp
- [ ] T043 [P] [US6] Create StatsPanel.qml with statistics overview display (counts, rates) in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/StatsPanel.qml
- [ ] T044 [P] [US6] Create CompletionRateChart.qml pie chart visualization using Qt Charts PieSeries in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/CompletionRateChart.qml
- [ ] T045 [P] [US6] Create DailyTrendChart.qml line chart showing last 7 days completion using Qt Charts LineSeries in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/DailyTrendChart.qml
- [ ] T046 [P] [US6] Create CategoryStatsChart.qml bar chart showing category distribution using Qt Charts BarSeries in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/CategoryStatsChart.qml
- [ ] T047 [US6] Add navigation to stats view from main window

**Checkpoint**: At this point, User Stories 1-6 should all work independently

---

## Phase 9: User Story 7 - 主题系统自定义 (Priority: P1)

**Goal**: Implement theme system with light/dark themes and custom theme creation

**Acceptance Criteria**:
- User can switch between light and dark themes
- User can create custom themes with custom colors (accent, background, text)
- Theme changes apply instantly to all UI components
- Theme selection persists across application restarts
- User can export/import custom themes as JSON files
- Default themes follow minimalist modern design style
- All QML components use theme properties for colors and styling

### Implementation for User Story 7

- [ ] T048 [P] [US7] Implement ThemeService (IThemeService) with light/dark/custom theme support, import/export in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/business/ThemeService.h and .cpp
- [ ] T049 [P] [US7] Create Theme.qml singleton with attached properties for QML theme bindings in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/themes/Theme.qml
- [ ] T050 [P] [US7] Create light theme definition (LightTheme.json) with color palette in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/themes/LightTheme.json
- [ ] T051 [P] [US7] Create dark theme definition (DarkTheme.json) with color palette in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/themes/DarkTheme.json
- [ ] T052 [P] [US7] Create ThemeSettings.qml for theme selection, custom theme editor, import/export in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/ThemeSettings.qml
- [ ] T053 [US7] Update all QML components (TodoItem, TodoForm, main.qml, etc.) to use Theme singleton properties

**Checkpoint**: At this point, User Stories 1-7 should all work independently

---

## Phase 10: User Story 8 - 动画效果 (Priority: P2)

**Goal**: Implement smooth animations and transitions for enhanced user experience

**Acceptance Criteria**:
- Todo items have smooth add/remove animations (fade + slide)
- Todo completion has satisfying checkmark animation
- Interactive components have hover and press feedback animations
- View transitions have smooth fade/slide animations
- Statistics counters animate when values change
- All animations run at 60fps
- Animations respect system "reduce motion" preferences if available

### Implementation for User Story 8

- [ ] T054 [P] [US8] Add smooth transitions for todo item add/remove/complete animations in TodoItem.qml using NumberAnimation and OpacityAnimator
- [ ] T055 [P] [US8] Add hover and press animations for interactive components (buttons, list items) using Behavior on scale/opacity
- [ ] T056 [P] [US8] Add page transition animations between main view, stats view, and settings view using StackView.push/pop transitions
- [ ] T057 [P] [US8] Add number animation for statistics counter updates in StatsPanel.qml using NumberAnimation with easing curve

**Checkpoint**: All user stories should now be independently functional

---

## Phase 11: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories and final integration

- [ ] T058 [P] Implement ConfigService (IConfigService) for centralized configuration management with persistence in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/business/ConfigService.h and .cpp
- [ ] T059 [P] Create SettingsDialog.qml combining all settings panels (GitHub, Theme, Reminder, Window) with tab navigation in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/qml/components/SettingsDialog.qml
- [ ] T060 [P] Add keyboard shortcuts support (Ctrl+N new todo, Ctrl+F search, Ctrl+S sync, etc.) using Shortcut and Action components
- [ ] T061 [P] Implement logging infrastructure with Qt logging categories and file output in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/src/data/Logger.h and .cpp
- [ ] T062 [P] Add comprehensive error handling and user-friendly error messages in all services
- [ ] T063 [P] Create application icon and asset files (ICO for Windows, ICNS for macOS) in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/assets/
- [ ] T064 Update CMakeLists.txt for packaging (Windows .exe with windeployqt, macOS .dmg with macdeployqt) using CPack
- [ ] T065 Create main.qrc Qt resource file bundling all QML, theme JSON, and asset files
- [ ] T066 Create README.md with build instructions, usage guide, and GitHub setup in repos/devinfra/boe_inspiration_wiki_wangpengjun_187/README.md

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3-10)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel (if staffed)
  - Or sequentially in priority order (P0 → P1 → P2)
- **Polish (Final Phase)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P0)**: Can start after Foundational (Phase 2) - No dependencies on other stories - **MVP**
- **User Story 2 (P0)**: Can start after Foundational (Phase 2) - Integrates with US1 data models but independently completable
- **User Story 3 (P0)**: Can start after Foundational (Phase 2) - Independent window features
- **User Story 4 (P1)**: Can start after Foundational (Phase 2) - Uses TodoService from US1 but extends it
- **User Story 5 (P1)**: Can start after Foundational (Phase 2) - Uses Todo data from US1
- **User Story 6 (P1)**: Can start after Foundational (Phase 2) - Queries Todo data from US1
- **User Story 7 (P1)**: Can start after Foundational (Phase 2) - Independent theme system
- **User Story 8 (P2)**: Can start after Foundational (Phase 2) - UI enhancements affecting all components

### Within Each User Story

- Models before services
- Services before UI components
- Core implementation before integration
- Story complete before moving to next priority

### Parallel Opportunities

- All Setup tasks marked [P] can run in parallel (T002, T003, T004)
- All Foundational tasks marked [P] can run in parallel (T005, T006, T007, T008, T009)
- Once Foundational phase completes, all user stories can start in parallel (if team capacity allows)
- Within US1: T010, T011, T012, T013, T014 can run in parallel
- Within US2: T017, T018, T019, T020, T021 can run in parallel
- Within US3: T024, T025, T026, T027 can run in parallel
- Within US4: T029, T030, T031, T032, T033 can run in parallel
- Within US5: T036, T037, T038, T039 can run in parallel
- Within US6: T042, T043, T044, T045, T046 can run in parallel
- Within US7: T048, T049, T050, T051, T052 can run in parallel
- Within US8: T054, T055, T056, T057 can run in parallel
- Within Polish: T058, T059, T060, T061, T062, T063 can run in parallel
- Different user stories can be worked on in parallel by different team members

---

## Parallel Example: User Story 1

```bash
# Launch all independent components for User Story 1 together:
Task T010: "Implement TodoService in src/business/TodoService.h and .cpp"
Task T011: "Create TodoListModel in src/models/TodoListModel.h and .cpp"
Task T012: "Create main.qml in src/qml/main.qml"
Task T013: "Create TodoItem.qml in src/qml/components/TodoItem.qml"
Task T014: "Create TodoForm.qml in src/qml/components/TodoForm.qml"
```

---

## Parallel Example: User Stories 1, 2, 3 (P0 Stories)

```bash
# With 3 developers, all P0 stories can be implemented in parallel after Foundational:
Developer A: User Story 1 (Todo CRUD + Priority) - T010 through T016
Developer B: User Story 2 (GitHub Sync) - T017 through T023
Developer C: User Story 3 (Desktop Widget) - T024 through T028
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001-T004)
2. Complete Phase 2: Foundational (T005-T009) - **CRITICAL - blocks all stories**
3. Complete Phase 3: User Story 1 (T010-T016)
4. **STOP and VALIDATE**: Verify User Story 1 works independently - core todo management is functional
5. Build and run basic smoke test on target platforms
6. Deploy/demo MVP if ready

### Incremental Delivery

1. Complete Setup + Foundational → Foundation ready
2. Add User Story 1 → Validate independently → Deploy/Demo (MVP: core todo management)
3. Add User Story 2 → Validate independently → Deploy/Demo (GitHub backup added)
4. Add User Story 3 → Validate independently → Deploy/Demo (Desktop widget features added)
5. Add User Story 4 → Validate independently → Deploy/Demo (Categories + Tags added)
6. Add User Story 5 → Validate independently → Deploy/Demo (Reminders + Tray added)
7. Add User Story 6 → Validate independently → Deploy/Demo (Statistics added)
8. Add User Story 7 → Validate independently → Deploy/Demo (Themes added)
9. Add User Story 8 → Validate independently → Deploy/Demo (Animations added)
10. Add Polish phase → Final release
11. Each story adds value without breaking previous stories

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together (or split parallel tasks)
2. Once Foundational is done:
   - Developer A: User Story 1 (Todo CRUD + Priority)
   - Developer B: User Story 2 (GitHub Sync)
   - Developer C: User Story 3 (Desktop Widget)
3. After P0 stories complete:
   - Developer A: User Story 4 (Categories + Tags)
   - Developer B: User Story 5 (Reminders + Tray)
   - Developer C: User Story 6 (Statistics)
4. Then:
   - Developer A: User Story 7 (Themes)
   - Developer B: User Story 8 (Animations)
   - Developer C: Polish tasks
5. Stories complete and integrate independently

---

## Task Summary

| Phase | Tasks | Count |
|-------|-------|-------|
| Phase 1: Setup | T001-T004 | 4 |
| Phase 2: Foundational | T005-T009 | 5 |
| Phase 3: US1 (Todo CRUD + Priority) | T010-T016 | 7 |
| Phase 4: US2 (GitHub Sync) | T017-T023 | 7 |
| Phase 5: US3 (Desktop Widget) | T024-T028 | 5 |
| Phase 6: US4 (Categories + Tags) | T029-T035 | 7 |
| Phase 7: US5 (Reminders + Tray) | T036-T041 | 6 |
| Phase 8: US6 (Statistics) | T042-T047 | 6 |
| Phase 9: US7 (Themes) | T048-T053 | 6 |
| Phase 10: US8 (Animations) | T054-T057 | 4 |
| Phase 11: Polish | T058-T066 | 9 |
| **Total** | | **66** |

### Parallel Opportunities Identified

- **Setup phase**: 3 parallel tasks (T002, T003, T004)
- **Foundational phase**: 5 parallel tasks (T005-T009)
- **US1**: 5 parallel tasks (T010-T014)
- **US2**: 5 parallel tasks (T017-T021)
- **US3**: 4 parallel tasks (T024-T027)
- **US4**: 5 parallel tasks (T029-T033)
- **US5**: 4 parallel tasks (T036-T039)
- **US6**: 5 parallel tasks (T042-T046)
- **US7**: 5 parallel tasks (T048-T052)
- **US8**: 4 parallel tasks (T054-T057)
- **Polish**: 6 parallel tasks (T058-T063)

### Suggested MVP Scope

**User Story 1 only** (Todo CRUD + Priority Management):
- Complete Phase 1 (Setup)
- Complete Phase 2 (Foundational)
- Complete Phase 3 (US1)
- This delivers a working todo application with core functionality that can be validated and demonstrated

### Acceptance Criteria per Story

| Story | Priority | Acceptance Criteria Met When |
|-------|----------|-------------------------------|
| US1 | P0 | Todo CRUD works, priorities display correctly, data persists to SQLite |
| US2 | P0 | GitHub sync works, conflicts detected, token encrypted, sync status shown |
| US3 | P0 | Frameless draggable window, always-on-top, opacity adjustable, position saved |
| US4 | P1 | Categories CRUD with colors, tags on todos, category filter works |
| US5 | P1 | Reminders trigger on due date, tray icon with menu, notifications work |
| US6 | P1 | Stats calculate correctly, charts render, auto-update on data change |
| US7 | P1 | Light/dark themes work, custom themes createable, instant theme switch |
| US8 | P2 | Smooth animations on add/remove/complete, hover effects, page transitions |

### Format Validation

All tasks follow the required checklist format:
- ✅ Start with `- [ ]` checkbox
- ✅ Sequential Task ID (T001-T066)
- ✅ [P] marker for parallelizable tasks
- ✅ [US#] story label for user story phase tasks
- ✅ Exact file paths in descriptions
- ✅ No unit test tasks included
- ✅ Test skill frontmatter prepended
- ✅ cli_version placeholder will be replaced on write

---

## Next Step

Run `flux-implement` (full mode) to begin implementation of the tasks above. Start with Phase 1 (Setup) and Phase 2 (Foundational) before moving to user stories.

<!-- cli_version: 0.1.41 -->
