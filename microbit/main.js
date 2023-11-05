radio.setGroup(133)
radio.setTransmitPower(7)

let parent = true

let currentTime = 0
let startTime = 1

let other = 0

let myScore = 0

let activeGame = false
let found = false

input.calibrateCompass()
pins.analogReadPin(AnalogPin.P20)
radio.onReceivedNumber(function (receivedNumber) {
    other = receivedNumber
})

basic.forever(function () {
    serial.writeNumber(input.compassHeading())

    if (activeGame) {
        if (!found) {
            if (!parent) {
                radio.sendNumber(input.compassHeading())

                if (other == -2) {
                    found = true
                }

                basic.showIcon(IconNames.Sad)
            }

            

            if (parent) {
                let angle = input.compassHeading() + other
                if (angle >= 240 && angle <= 300) {
                    basic.showIcon(IconNames.Heart)
                    radio.sendNumber(-2)
                    found = true
                }
                else {
                    radio.sendNumber(-3)
                    basic.showIcon(IconNames.Sad)
                }
            }
        }
        else {
            basic.showIcon(IconNames.Yes)
        }
    }
    else {
        if (!parent) {
            radio.sendNumber(-1)
        }

        basic.showIcon(IconNames.No)

        if (other == -1) {
            activeGame = true
            music.tonePlayable(Note.C, music.beat(BeatFraction.Whole))
        }
    }

    if (parent) {
        led.plot(0, 0)
    }
})

loops.everyInterval(1000, function () {
    if (parent && other == -1) {
        if (!activeGame) {
            currentTime += 1
        }

        if (currentTime >= startTime) {
            radio.sendNumber(-1)
            activeGame = true
            music.tonePlayable(Note.C, music.beat(BeatFraction.Whole))
        }
    }
})