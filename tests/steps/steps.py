# -*- coding: UTF-8 -*-
from behave import step, then

from common_steps import *
from time import sleep
from dogtail.rawinput import keyCombo
from subprocess import Popen, PIPE
from dogtail import i18n


@step(u'Open About dialog')
def open_about_dialog(context):
    context.app.menu(translate('Help')).click()
    context.app.menu(translate('Help')).menuItem(translate('About')).click()
    context.about_dialog = context.app.dialog(translate('About Image Viewer'))


@then(u'Website link to wiki is displayed')
def website_link_to_wiki_is_displayed(context):
    assert context.about_dialog.child(translate('Website')).showing


@then(u'GPL 2.0 link is displayed')
def gpl_license_link_is_displayed(context):
    assert context.about_dialog.child(translate("Image Viewer")).showing, "App name is not displayed"
    assert context.about_dialog.child(translate("The GNOME image viewer.")).showing, "App description is not displayed"
    assert context.about_dialog.child(translate("Website")).showing, "Website link is not displayed"
    assert context.about_dialog.child(roleName='radio button', name=translate("About")).checked, "About tab is not selected"
    assert not context.about_dialog.child(roleName='radio button', name=translate("Credits")).checked, "Credits tab is selected"


@step(u'Open "{filename}" via menu')
def open_file_via_menu(context, filename):
    keyCombo("<Ctrl>O")
    context.execute_steps(u"""
        * file select dialog with name "Open Image" is displayed
        * In file select dialog select "%s"
    """ % filename)
    sleep(0.5)


@then(u'image size is {width:d}x{height:d}')
def image_size_is(context, width, height):
    for attempt in xrange(0, 10):
        width_text = context.app.child(roleName='page tab list').child(translate('Width:')).parent.children[-1].text
        if width_text == '':
            sleep(0.5)
            continue
        else:
            break
    height_text = context.app.child(roleName='page tab list').child(translate('Height:')).parent.children[-1].text
    try:
        actual_width = int(width_text.split(' ')[0])
        actual_height = int(height_text.split(' ')[0])
    except Exception:
        raise Exception("Incorrect width/height is been displayed")
    assert actual_width == width
    assert actual_height == height


@step(u'Rotate the image clockwise')
def rotate_image_clockwise(context):
    context.app.child(roleName='tool bar').child(translate('Right')).click()


@step(u'Open context menu for current image')
def open_context_menu(context):
    context.app.child(roleName='drawing area').click(button=3)
    sleep(0.1)


@step(u'Select "{item}" from context menu')
def select_item_from_context_menu(context, item):
    context.app.child(roleName='drawing area').click(button=3)
    sleep(0.1)
    context.app.child(roleName='window').menuItem(item).click()


@then(u'sidepanel is {state:w}')
def sidepanel_displayed(context, state):
    sleep(0.5)
    assert state in ['displayed', 'hidden'], "Incorrect state: %s" % state
    actual = context.app.child(roleName='page tab list').showing
    assert actual == (state == 'displayed')


def app_is_not_fullscreen(context):
    import ipdb; ipdb.set_trace()


@then(u'application is {negative:w} fullscreen anymore')
@then(u'application is displayed fullscreen')
def app_displayed_fullscreen(context, negative=None):
    sleep(0.5)
    actual = not context.app.child(roleName='menu bar').showing
    assert actual == (negative is None)


@step(u'Wait a second')
def wait_a_second(context):
    sleep(1)


@step(u'Click "Hide" in wallpaper popup')
def hide_wallapper_popup(context):
    context.app.button(translate('Hide')).click()


@then(u'wallpaper is set to "{filename}"')
def wallpaper_is_set_to(context, filename):
    wallpaper_path = Popen(["gsettings", "get", "org.gnome.desktop.background", "picture-uri"], stdout=PIPE).stdout.read()
    actual_filename = wallpaper_path.split('/')[-1].split("'")[0]
    assert filename == actual_filename


@then(u'"{filename}" file exists')
def file_exists(context, filename):
    assert os.path.isfile(os.path.expanduser(filename))


@then(u'image type is "{mimetype}"')
def image_type_is(context, mimetype):
    imagetype = context.app.child(roleName='page tab list').child(translate('Type:')).parent.children[-1].text
    assert imagetype == mimetype


@step(u'Select "{menu}" menu')
def select_menuitem(context, menu):
    menu_item = menu.split(' -> ')
    # First level menu
    current = context.app.menu(translate(menu_item[0]))
    current.click()
    if len(menu_item) == 1:
        return
    # Intermediate menus - point them but don't click
    for item in menu_item[1:-1]:
        current = context.app.menu(translate(item))
        current.point()
    # Last level menu item
    current.menuItem(translate(menu_item[-1])).click()


@step(u'Select and close "{menu}" menu')
def select_menuitem_and_close_it(context, menu):
    context.execute_steps('* Select "%s" menu' % menu)
    keyCombo("<Esc>")


@step(u'Open "Image -> Save As" menu')
def open_save_as_menu(context):
    context.app.menu(translate("Image")).click()
    context.app.menu(translate("Image")).findChildren(lambda x: 'Save As' in x.name)[0].click()


@step(u'Select "{name}" window')
def select_name_window(context, name):
    context.app = context.app.child(roleName='frame', name=translate(name))
    context.app.grab_focus()
