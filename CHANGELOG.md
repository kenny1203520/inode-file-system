# Changelog

All notable changes to this project will be documented in this file.

## [0.2.0] - 2026-01-11

### Added
- **Complete GUI Implementation**:
  - Implemented main window with file list, path label, and status bar.
  - Added toolbar with "Up" and "Refresh" buttons.
  - Implemented context menus for file/directory operations and background operations.
  - Added dialogs for file creation, renaming, and properties.
- **File Operations**:
  - Added ability to create new files and directories via GUI.
  - Implemented Copy, Copy/Move (Cut), and Paste functionality.
  - Added recursive directory deletion with confirmation.
  - Added file renaming with validation.
  - Implemented recursive directory size calculation for properties view.
- **Shell Support**:
  - Added `cp` (copy) and `mv` (move) commands to the CLI shell.
- **Documentation**:
  - Added comprehensive `FEATURES.md` describing all GUI functionalities and code structure.

### Fixed
- **Shell**: Fixed command case sensitivity issues.
- **Localization**: Fixed Chinese text errors.
- Fixed text editor functionality issues.
- Improved path handling and navigation logic in the GUI.

## [0.1.0]

- Initial release of the inode file system with basic CLI support.
