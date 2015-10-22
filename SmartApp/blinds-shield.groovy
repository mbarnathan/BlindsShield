/**
 *  Copyright 2015 Michael Barnathan
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 */
metadata {
    definition (name: "Blinds Shield", namespace: "quantiletree", author: "Michael Barnathan") {
        capability "Switch Level"
        capability "Actuator"
        capability "Switch"
        capability "Configuration"
        capability "Sensor"
        capability "Refresh"
    }

    // Simulator metadata
    simulator {
        status "down":  "switch: on"
        status "up": "switch: off"

        // reply messages
        reply "raw 0x0 { 00 00 0a 0a 6f 6e }": "catchall: 0104 0000 01 01 0040 00 0A21 00 00 0000 0A 00 0A6F6E"
        reply "raw 0x0 { 00 00 0a 0a 6f 66 66 }": "catchall: 0104 0000 01 01 0040 00 0A21 00 00 0000 0A 00 0A6F6666"
    }

    tiles(scale: 2) {
        multiAttributeTile(name:"switch", type: "lighting", width: 6, height: 4, canChangeIcon: true) {
            tileAttribute ("device.switch", key: "PRIMARY_CONTROL") {
                attributeState "on", label:'down', action:"switch.off", icon:"st.Weather.weather15", backgroundColor:"#555555", nextState:"turningOff"
                attributeState "off", label:'up', action:"switch.on", icon:"st.Weather.weather14", backgroundColor:"#3bbcff", nextState:"turningOn"
                attributeState "turningOn", label:'lowering', action:"switch.off", icon:"st.Weather.weather11", backgroundColor:"#777777", nextState: "turningOff"
                attributeState "turningOff", label:'raising', action:"switch.on", icon:"st.Weather.weather11", backgroundColor:"#2287cd", nextState: "turningOn"
            }
            tileAttribute ("device.level", key: "SLIDER_CONTROL") {
                attributeState "level", action:"switch level.setLevel"
            }
        }
        standardTile("refresh", "device.switch", inactiveLabel: false, decoration: "flat", width: 2, height: 2) {
            state "default", label:"", action:"refresh.refresh", icon:"st.secondary.refresh"
        }
        valueTile("level", "device.level", inactiveLabel: false, decoration: "flat", width: 2, height: 2) {
            state "level", label:'${currentValue} %', unit:"%", backgroundColor:"#ffffff"
        }
        main "switch"
        details(["switch", "refresh", "level", "levelSliderControl"])
    }
}

// Parse incoming device messages to generate events
def parse(String description) {
    def value = zigbee.parse(description)?.text
    def name = null
    if (value in ["on","off","turningOn","turningOff"]) {
        name = "switch"
    } else if (value.startsWith("level ")) {
        name = "level"
        value = value.split(" ")[-1]
    }
    def result = createEvent(name: name, value: value)
    log.debug "Parse returned ${result?.descriptionText}"
    return result
}

// Commands sent to the device
def on() {
    zigbee.smartShield(text: "on").format()
}

def off() {
    zigbee.smartShield(text: "off").format()
}

def setLevel(value) {
    log.trace "setLevel($value)"
    zigbee.smartShield(text: "$value").format()
}

def refresh() {
    zigbee.smartShield(text: "refresh").format()
}
