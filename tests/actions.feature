Feature: Smoke tests

  Background:
    * Make sure that eog is running

  @about
  Scenario: About dialog
    * Open About dialog
    Then Website link to wiki is displayed
     And GPL 2.0 link is displayed

  @undo @undo_via_toolbar
  Scenario: Undo via toolbar
    * Open "/tmp/gnome-logo.png" via menu
    Then image size is 199x76
    * Rotate the image clockwise
    Then image size is 76x199
    * Select "Edit -> Undo" menu
    Then image size is 199x76

  @undo @undo_via_shortcut
  Scenario: Undo via shortcut
    * Open "/tmp/gnome-logo.png" via menu
    Then image size is 199x76
    * Rotate the image clockwise
    Then image size is 76x199
    * Press "<Ctrl>z"
    Then image size is 199x76

  @sidepane @sidepane_via_menu
  Scenario: Sidepanel via menu
    * Open "/tmp/gnome-logo.png" via menu
    Then sidepanel is displayed
    * Select "View -> Side Pane" menu
    Then sidepanel is hidden

  @sidepane @sidepane_via_shortcut
  Scenario: Slideshow via shortcut
    * Open "/tmp/gnome-logo.png" via menu
    * Press "<Ctrl><F9>"
    Then sidepanel is hidden
    * Select "View -> Side Pane" menu
    Then sidepanel is displayed

  @fullscreen @fullscreen_via_menu
  Scenario: Fullscreen via menu
    * Open "/tmp/gnome-logo.png" via menu
    * Select "View -> Fullscreen" menu
    Then application is displayed fullscreen
    * Press "<Esc>"
    Then application is not fullscreen anymore

  @fullscreen @fullscreen_via_shortcut
  Scenario: Fullscreen via shortcut
    * Open "/tmp/gnome-logo.png" via menu
    * Press "<F11>"
    Then application is displayed fullscreen
    * Press "<Esc>"
    Then application is not fullscreen anymore

  @wallpaper
  Scenario: Set as wallpaper
    * Open "/tmp/gnome-logo.png" via menu
    * Select "Set as Wallpaper" from context menu
    * Click "Hide" in wallpaper popup
    Then wallpaper is set to "gnome-logo.png"

