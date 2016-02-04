Feature: Screenshot tour

    @screenshot_tour1
    Scenario Outline: Main dialogs
     * Set locale to "<locale>"
     * Make sure that xviewer is running
     * Select and close "Image" menu
     * Select and close "Edit" menu
     * Select and close "View" menu
     * Select and close "Go" menu
     * Select and close "Help" menu
     * Open "/tmp/gnome-logo.png" via menu
     * Open context menu for current image

    Examples:
     | locale |
     | es_ES  |
     | el_GR  |
     | lt_LT  |
     | gl_ES  |
     | cs_CZ  |
     | ru_RU  |
     | id_ID  |

    @screenshot_tour2
    Scenario Outline: Main dialogs
     * Set locale to "<locale>"
     * Make sure that xviewer is running
     * Select and close "Image" menu
     * Select and close "Edit" menu
     * Select and close "View" menu
     * Select and close "Go" menu
     * Select and close "Help" menu
     * Open "/tmp/gnome-logo.png" via menu
     * Open context menu for current image

    Examples:
     | locale |
     | hu_HU  |
     | pl_PL  |
     | fr_FR  |
     | sl_SI  |
     | it_IT  |
     | da_DK  |
     | de_DE  |

     # Error selecting translations
     #| pt_BR  |
     #| zh_TW  |
     #| ca_ES  |
     #| sr_RS  |
     #| sr_RS@latin |

     # Missing fonts:
     # | zh_CN  |