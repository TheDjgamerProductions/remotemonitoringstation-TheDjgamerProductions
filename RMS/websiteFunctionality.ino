void routesConfiguration() {

  // Example of a 'standard' route
  // No Authentication
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    logEvent("route: /");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Duplicated serving of index.html route, so the IP can be entered directly to browser.
  // No Authentication
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    logEvent("route: /");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Example of linking to an external file
  server.on("/arduino.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/arduino.css", "text/css");
  });


  // Example of a route with additional authentication (popup in browser)
  // And uses the processor function.
  server.on("/dashboard.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    logEvent("Dashboard");
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });

  server.on("/adminDashboard.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(admin_username, admin_password))
      return request->requestAuthentication();
    logEvent("Admin Login");
    request->send(SPIFFS, "/adminDashboard.html", "text/html", false, processor);
  });




  server.on("/toggleLED", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    LEDOn = !LEDOn;
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });


  // Example of route which sets file to download - 'true' in send() command.
  server.on("/logOutput", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(admin_username, admin_password))
      return request->requestAuthentication();
    logEvent("Log Event Download");
    request->send(SPIFFS, "/logEvents.csv", "text/html", true);
  });



  server.on("/ToggleFan",  HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    fanEnabled = !fanEnabled;
    logEvent("Fan toggled via Website");
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });

  server.on("/FanControl",  HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    autoFanEnabled = !autoFanEnabled;
    logEvent("Fan state toggled via Website");
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });

  server.on("/ToggleSafe",  HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    safeLocked = !safeLocked;
    logEvent("Safe toggled via Website");
    request->send(SPIFFS, "/dashboard.html", "text/html", false, processor);
  });


  server.onNotFound([](AsyncWebServerRequest * request) {
    if (request->url().endsWith(F(".jpg"))) {
      // Extract the filename that was attempted
      int fnsstart = request->url().lastIndexOf('/');
      String fn = request->url().substring(fnsstart);
      // Load the image from SPIFFS and send to the browser.
      request->send(SPIFFS, fn, "image/jpeg", true);
    } else {
      request->send(SPIFFS, "/404.html");
    }
  });
}

String getDateTime() {
  DateTime rightNow = rtc.now();
  char csvReadableDate[25];
  sprintf(csvReadableDate, "%02d:%02d:%02d %02d/%02d/%02d",  rightNow.hour(), rightNow.minute(), rightNow.second(), rightNow.day(), rightNow.month(), rightNow.year());
  return csvReadableDate;
}

String processor(const String& var) {
  /*
     Updates the HTML by replacing set variables with return value from here.
     For example:
     in HTML file include %VARIABLEVALUE%.
     In this function, have:
      if (var=="VARIABLEVALUE") { return "5";}
  */

  if (var == "DATETIME") {
    String datetime = getDateTime();
    return datetime;
  }
  if (var == "INVFANSTATE") {
    if (fanEnabled) {
      return "Off";
    } else {
      return "On";
    }
  }

  if (var == "TEMPERATURE") {
    return String(tempsensor.readTempC());
  }

  if (var == "FANCONTROL") {
    if (autoFanEnabled) {
      return "Automatic";
    } else {
      return "Manual";
    }
  }
  if (var == "INVFANCONTROL") {
    if (autoFanEnabled) {
      return "Manual";
    } else {
      return "Automatic";
    }
  }

  if (var == "SAFESTATE") {
    if (safeLocked) {
      return "Locked";
    }
    else {
      return "Unlocked";
    }
  }
  if (var == "TOGGLESAFESTATE") {
    if (safeLocked) {
      return "Unlock Safe";
    }
    else {
      return "Lock Safe";
    }
  }

  if (var == "TOGGLELED") {
    if (LEDOn) {
      return "Off";
    }
    else {
      return "On";
    }
    
  }

  // Default "catch" which will return nothing in case the HTML has no variable to replace.
  return String();
}
