import QtQuick
import QtQuick.Layouts
import MangaDl

// Create account, with live password strength and match checks.
AuthLayout {
    id: page
    eyebrow: "Step 3 of 3"
    title: "Create account"
    subtitle: "Free. Keeps your library, history and backups in sync across devices."
    formSpacing: 14

    readonly property string pw: password.text
    readonly property bool matches: pw === confirm.text
    readonly property int score: (pw.length >= 8) + /[A-Za-z]/.test(pw) + /[0-9]/.test(pw) + (/[^A-Za-z0-9]/.test(pw) || pw.length >= 12)
    readonly property int bars: Math.min(4, score)

    Connections {
        target: AccountController
        function onAuthSuccess(action) {
            AppSettings.onboardingCompleted = true
            Nav.toast("Account created for " + (AccountController.email || username.text))
            Nav.home()
        }
        function onErrorMessageChanged() {
            if (AccountController.errorMessage !== "") {
                Nav.toast("Sign up error: " + AccountController.errorMessage)
            }
        }
    }

    LabeledField {
        id: username
        label: "Username"
        placeholderText: "Choose a username"
        onAccepted: email.forceActiveFocus()
    }
    LabeledField {
        id: email
        label: "Email"
        placeholderText: "you@example.com"
        onAccepted: password.forceActiveFocus()
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        LabeledField {
            id: password
            label: "Password"
            placeholderText: "Password"
            password: true
            Layout.preferredWidth: 1
            onAccepted: confirm.forceActiveFocus()
        }
        LabeledField {
            id: confirm
            label: "Confirm"
            placeholderText: "Confirm"
            password: true
            error: !page.matches
            Layout.preferredWidth: 1
            onAccepted: {
                if (!AccountController.isBusy && terms.checked && page.matches && page.pw.length > 0 && username.text.length > 0 && email.text.length > 0) {
                    AccountController.signUp(username.text, email.text, password.text)
                }
            }
        }
    }
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: 4
                Rectangle {
                    required property int index
                    Layout.fillWidth: true
                    height: 4; radius: 2
                    color: index < page.bars ? (page.bars >= 3 ? Theme.success : Theme.accent) : Theme.line
                }
            }
        }
        Txt {
            text: !page.matches ? "Passwords don't match." : page.bars >= 3 ? "Strong password · passwords match" : "Weak password · passwords match"
            font.pixelSize: 12; font.weight: Font.DemiBold
            color: page.matches && page.bars >= 3 ? Theme.successText : Theme.errorText
        }
    }
    Txt {
        visible: AccountController.errorMessage !== ""
        text: AccountController.errorMessage
        color: Theme.errorText
        font.pixelSize: 13
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }
    Row {
        CheckRow { id: terms; text: "I agree to the"; checked: true; font.pixelSize: 13 }
        Btn { kind: "link"; text: "Terms"; fontSize: 13; anchors.verticalCenter: parent.verticalCenter }
    }
    Btn {
        Layout.fillWidth: true; size: "large"
        text: AccountController.isBusy ? "Creating Account..." : "Create Account"
        enabled: !AccountController.isBusy && terms.checked && page.matches && page.pw.length > 0 && username.text.length > 0 && email.text.length > 0
        onClicked: AccountController.signUp(username.text, email.text, password.text)
    }
    Row {
        Layout.topMargin: 6
        BodyMuted { text: "Have an account?"; anchors.verticalCenter: parent.verticalCenter }
        Btn { kind: "link"; text: "Sign in"; onClicked: Nav.go("Login") }
    }
}
