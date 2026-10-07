import QtQuick 2.15

QtObject {
    property var vehicle
    property int currentPage: 0
    property bool active: false

    function openPage(index) {
        currentPage = (index + 5) % 5
        active = true
    }

    function handleAction(action) {
        if (action === "OPEN_MENU") {
            active = !active
        } else if (action === "BACK") {
            active = false
        } else if (action === "HOME" || action === "OPEN_DRIVE") {
            currentPage = 0
            active = false
        } else if (action === "OPEN_ENERGY") {
            openPage(1)
        } else if (action === "OPEN_DISPLAY") {
            openPage(4)
        } else if (!active) {
            return
        } else if (action === "NAV_LEFT") {
            openPage(currentPage - 1)
        } else if (action === "NAV_RIGHT") {
            openPage(currentPage + 1)
        } else if (action === "NAV_UP" || action === "NAV_DOWN") {
            const amount = action === "NAV_UP" ? 1 : -1
            if (currentPage === 0) vehicle.setRegenLevel(vehicle.regenLevel + amount)
            if (currentPage === 4) vehicle.changeBrightness(amount * 10)
        } else if (action === "SELECT") {
            if (currentPage === 2) vehicle.resetTrip()
            active = false
        }
    }
}
