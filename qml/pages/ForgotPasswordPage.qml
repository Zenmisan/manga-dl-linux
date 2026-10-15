import QtQuick
import QtQuick.Layouts
import MangaDl

// Forgot password: email form, then a "link sent" state with a resend countdown.
AuthLayout {
    id: page
    title: "Reset password"
    subtitle: "Enter your account email. We'll send a link to set a new password."
    property bool sent: false
    property int countdown: 45

    Timer { running: page.sent && page.countdown > 0; interval: 1000; repeat: true; onTriggered: page.countdown-- }

    Connections {
        target: AccountController
        function onAuthSuccess(action) {
            if (action === "password_reset") {
                page.sent = true
                Nav.toast("Password reset link sent to " + emailField.text)
            }
        }
        function onErrorMessageChanged() {
            if (AccountController.errorMessage !== "") {
                Nav.toast(AccountController.errorMessage)
            }
        }
    }

    Btn { kind: "link"; text: "Back to Sign In"; fontSize: 13; iconName: "Back"; iconSize: 14; onClicked: Nav.back() }
    LabeledField { id: emailField; label: "Email"; placeholderText: "you@example.com" }

    Txt {
        visible: AccountController.errorMessage !== ""
        text: AccountController.errorMessage
        color: Theme.errorText
        font.pixelSize: 13
        wrapMode: Text.Wrap
        Layout.fillWidth: true
    }

    Btn {
        visible: !page.sent; Layout.fillWidth: true; size: "large"
        text: AccountController.isBusy ? "Sending..." : "Send Reset Link"
        enabled: !AccountController.isBusy && emailField.text.length > 0
        onClicked: AccountController.requestPasswordReset(emailField.text)
    }

    Btn { visible: page.sent; Layout.fillWidth: true; kind: "success"; size: "large"; text: "Link Sent"; iconName: "Check"; iconSize: 18; enabled: false; opacity: 1 }
    Card {
        visible: page.sent
        Layout.fillWidth: true
        kind: "list"; padding: 16
        Txt {
            Layout.fillWidth: true
            text: "If an account exists for that email, a reset link is on its way. Check your spam folder too."
            color: Theme.fgStrong; wrapMode: Text.Wrap; lineHeightMode: Text.FixedHeight; lineHeight: 21
        }
    }
    Btn {
        visible: page.sent
        kind: "link"; fontSize: 13
        enabled: page.countdown === 0 && !AccountController.isBusy
        textColor: enabled ? Theme.accentLight : Theme.fgMuted
        text: page.countdown > 0 ? "Resend in 0:" + ("0" + page.countdown).slice(-2) : "Resend link"
        onClicked: {
            page.countdown = 45
            AccountController.requestPasswordReset(emailField.text)
        }
    }
}
