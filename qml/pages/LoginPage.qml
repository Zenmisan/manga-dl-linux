import QtQuick
import QtQuick.Layouts
import MangaDl

// Sign in.
AuthLayout {
    title: "Sign in"
    titleSize: 132
    subtitle: "Sync your library, history and backups between this computer, your phone and the web."

    Connections {
        target: AccountController
        function onAuthSuccess(action) {
            Nav.toast("Signed in as " + (AccountController.email || AccountController.displayName || "user"))
            Nav.home()
        }
        function onErrorMessageChanged() {
            if (AccountController.errorMessage !== "") {
                Nav.toast("Sign in failed: " + AccountController.errorMessage)
            }
        }
    }

    LabeledField { id: emailField; label: "Email"; placeholderText: "you@example.com" }
    LabeledField { id: passwordField; label: "Password"; placeholderText: "Your password"; password: true }
    RowLayout {
        Layout.fillWidth: true
        CheckRow { text: "Stay signed in"; checked: true; font.pixelSize: 13; Layout.fillWidth: true }
        Btn { kind: "link"; text: "Forgot password?"; fontSize: 13; onClicked: Nav.open("ForgotPassword") }
    }
    Txt {
        visible: AccountController.errorMessage !== ""
        text: AccountController.errorMessage
        color: Theme.errorText
        font.pixelSize: 13
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    Btn {
        Layout.fillWidth: true; size: "large"
        text: AccountController.isBusy ? "Signing in..." : "Sign In"
        enabled: !AccountController.isBusy && emailField.text.length > 0 && passwordField.text.length > 0
        onClicked: AccountController.signIn(emailField.text, passwordField.text)
    }
    Btn {
        Layout.fillWidth: true; kind: "ghost"; size: "large"; text: "Continue with Google"
        enabled: !AccountController.isBusy
        onClicked: AccountController.signInWithGoogle()
    }
    Btn { Layout.fillWidth: true; kind: "ghost"; size: "large"; text: "Use Without Account"; onClicked: Nav.home() }
    Row {
        Layout.topMargin: 8
        BodyMuted { text: "No account?"; anchors.verticalCenter: parent.verticalCenter }
        Btn { kind: "link"; text: "Create one"; onClicked: Nav.go("Register") }
    }
}
