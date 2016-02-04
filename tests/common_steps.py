# -*- coding: UTF-8 -*-
from dogtail.utils import isA11yEnabled, enableA11y
if isA11yEnabled() is False:
    enableA11y(True)

from time import time, sleep
from functools import wraps
from os import strerror, errno, kill, system, path, getcwd, environ
from signal import signal, alarm, SIGALRM, SIGKILL
from subprocess import Popen
from behave import step
from gi.repository import GLib

from dogtail.rawinput import keyCombo, absoluteMotion, pressKey
from dogtail.tree import root
from dogtail.utils import run
from dogtail.predicate import GenericPredicate
from dogtail import i18n
import pyatspi


def cleanup():
    for schema in ['org.x.viewer.fullscreen', 'org.x.viewer.plugins', 'org.x.viewer.ui', 'org.x.viewer.view']:
        system("gsettings reset-recursively %s" % schema)

    # Remove all the remains of other files
    system("rm /tmp/gnome-logo.bmp -rf")

    # Make sure we have a test file present
    testfile_path = path.join(path.dirname(path.realpath(__file__)), "gnome-logo.png")
    system("cp %s /tmp" % testfile_path)


def wait_until(my_lambda, element, timeout=30, period=0.25):
    """
    This function keeps running lambda with specified params until the result is True
    or timeout is reached
    Sample usages:
     * wait_until(lambda x: x.name != 'Loading...', context.app.instance)
       Pause until window title is not 'Loading...'.
       Return False if window title is still 'Loading...'
       Throw an exception if window doesn't exist after default timeout

     * wait_until(lambda element, expected: x.text == expected, element, ('Expected text'))
       Wait until element text becomes the expected (passed to the lambda)

    """
    exception_thrown = None
    mustend = int(time()) + timeout
    while int(time()) < mustend:
        try:
            if my_lambda(element):
                return True
        except Exception as e:
            # If lambda has thrown the exception we'll re-raise it later
            # and forget about if lambda passes
            exception_thrown = e
        sleep(period)
    if exception_thrown:
        raise exception_thrown
    else:
        return False


class TimeoutError(Exception):
    """
    Timeout exception class for limit_execution_time_to function
    """
    pass


def limit_execution_time_to(
        seconds=10, error_message=strerror(errno.ETIME)):
    """
    Decorator to limit function execution to specified limit
    """
    def decorator(func):
        def _handle_timeout(signum, frame):
            raise TimeoutError(error_message)

        def wrapper(*args, **kwargs):
            signal(SIGALRM, _handle_timeout)
            alarm(seconds)
            try:
                result = func(*args, **kwargs)
            finally:
                alarm(0)
            return result

        return wraps(func)(wrapper)

    return decorator


class App(object):
    """
    This class does all basic events with the app
    """
    def __init__(
        self, appName, shortcut='<Control><Q>', a11yAppName=None,
            forceKill=True, parameters='', recordVideo=False):
        """
        Initialize object App
        appName     command to run the app
        shortcut    default quit shortcut
        a11yAppName app's a11y name is different than binary
        forceKill   is the app supposed to be kill before/after test?
        parameters  has the app any params needed to start? (only for startViaCommand)
        recordVideo start gnome-shell recording while running the app
        """
        self.appCommand = appName
        self.shortcut = shortcut
        self.forceKill = forceKill
        self.parameters = parameters
        self.internCommand = self.appCommand.lower()
        self.a11yAppName = a11yAppName
        self.recordVideo = recordVideo
        self.pid = None

        # a way of overcoming overview autospawn when mouse in 1,1 from start
        pressKey('Esc')
        absoluteMotion(100, 100, 2)

        # attempt to make a recording of the test
        if self.recordVideo:
            keyCombo('<Control><Alt><Shift>R')

    def isRunning(self):
        """
        Is the app running?
        """
        if self.a11yAppName is None:
            self.a11yAppName = self.internCommand

        # Trap weird bus errors
        for attempt in xrange(0, 10):
            try:
                return self.a11yAppName in [x.name for x in root.applications()]
            except GLib.GError:
                continue
        raise Exception("10 at-spi errors, seems that bus is blocked")

    def kill(self):
        """
        Kill the app via 'killall'
        """
        if self.recordVideo:
            keyCombo('<Control><Alt><Shift>R')

        try:
            kill(self.pid, SIGKILL)
        except:
            # Fall back to killall
            Popen("killall " + self.appCommand, shell=True).wait()

    def startViaCommand(self):
        """
        Start the app via command
        """
        if self.forceKill and self.isRunning():
            self.kill()
            assert not self.isRunning(), "Application cannot be stopped"

        command = "%s %s" % (self.appCommand, self.parameters)
        self.pid = run(command, timeout=1)

        assert self.isRunning(), "Application failed to start"
        return root.application(self.a11yAppName)

    def closeViaShortcut(self):
        """
        Close the app via shortcut
        """
        if not self.isRunning():
            raise Exception("App is not running")

        keyCombo(self.shortcut)
        assert not self.isRunning(), "Application cannot be stopped"


@step(u'Make sure that {app} is running')
def ensure_app_running(context, app):
    context.app = context.app_class.startViaCommand()


@step(u'Press "{sequence}"')
def press_button_sequence(context, sequence):
    keyCombo(sequence)
    sleep(0.5)


@step(u'Folder select dialog with name "{name}" is displayed')
def has_folder_select_dialog_with_name(context, name):
    has_files_select_dialog_with_name(context, name)


@step(u'Folder select dialog is displayed')
def has_folder_select_dialog(context):
    context.execute_steps(
        u'Then folder select dialog with name "Select Folder" is displayed')


@step(u'In folder select dialog choose "{name}"')
def select_folder_in_dialog(context, name):
    select_file_in_dialog(context, name)


@step(u'file select dialog with name "{name}" is displayed')
def has_files_select_dialog_with_name(context, name):
    context.app.dialog = context.app.child(name=translate(name),
                                           roleName='file chooser')


@step(u'File select dialog is displayed')
def has_files_select_dialog(context):
    context.execute_steps(
        u'Then file select dialog with name "Select Files" is displayed')


@step(u'In file select dialog select "{name}"')
def select_file_in_dialog(context, name):
    # Find an appropriate button to click
    # It will be either 'Home' or 'File System'

    home_folder = context.app.dialog.findChild(GenericPredicate(name=translate('Home')),
                                               retry=False,
                                               requireResult=False)
    if home_folder:
        home_folder.click()
    else:
        context.app.dialog.childNamed(translate('File System')).click()
    location_button = context.app.dialog.child(translate('Enter Location'))
    if not pyatspi.STATE_SELECTED in location_button.getState().getStates():
        location_button.click()

    location_text = context.app.dialog.child(roleName='text')
    location_text.set_text_contents(name)
    sleep(0.2)
    location_text.grab_focus()
    keyCombo('<Enter>')
    assert wait_until(lambda x: x.dead, context.app.dialog), "Dialog was not closed"


@step(u'In file save dialog save file to "{path}" clicking "{button}"')
def file_save_to_path(context, path, button):
    context.app.dialog.childLabelled(translate('Name:')).set_text_contents(path)
    context.app.dialog.childNamed(button).click()
    assert wait_until(lambda x: x.dead, context.app.dialog), "Dialog was not closed"


@step(u'Set locale to "{locale}"')
def set_locale_to(context, locale):
    environ['LANG'] = locale
    i18n.translationDbs = []
    i18n.loadTranslationsFromPackageMoFiles('xviewer')
    i18n.loadTranslationsFromPackageMoFiles('gtk30')

    context.current_locale = locale
    context.screenshot_counter = 0


def translate(string):
    translation = i18n.translate(string)
    if translation == []:
        translation = string
    else:
        if len(translation) > 1:
            print("Options for '%s'" % string)
            print(translation)
        translation = translation[-1].decode('utf-8')
    return translation
