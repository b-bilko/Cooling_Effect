#pragma once

// Wi-Fi access point + web page for configuring the cool-down.

void portalBegin();
void portalLoop();
void portalEnd();

// True while a phone/laptop is joined to the access point, or a browser has
// talked to the portal recently (the page polls every couple of seconds).
bool portalActive();
