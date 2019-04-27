#include <Arduino.h>
#include "Pages.h"

String renderAlarm() {
    return "<!DOCTYPE HTML><html><head><meta charset='UTF-8'></head><body><h1>End of World</h1> <br> <h2>Yes!</h2><h2>A B Konec</h2><style>h1{color:red}</style></body></html>";
}

String renderTest(String CAS, String CAS_DOSEL) {
    return "<!DOCTYPE HTML><html><head><meta charset='UTF-8'></head><body><h1>Je cas: " + CAS + " " + CAS_DOSEL + " </h1></body></html>";
}

