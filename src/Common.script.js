
function newline(device, online, progress, context) {
    // input contains c++ like control chars (\n,\\)
    var text = device.getParameterByName(context.textbox);
    var replaced = text.value.split("\\n").join("\n");
    text.value = replaced;
}

function BASE_getUnsupportedEtsModules(device, online, progress, context) {
    var sync = context.Sync;

    progress.setText("Common: Frage Hardware nach unterstützten Modulen...");
    progress.setProgress(1);
    online.connect();
    progress.setProgress(20);
    
    var data = [0]; // no input data
    var resp = online.invokeFunctionProperty(158, 2, data);
    online.disconnect();

    if (!resp || resp.length < 1 || resp[0] != 0) {
        throw new Error("Common: Keine Antwort vom Gerät!");
    }
    if (resp.length < 5) { // error
        throw new Error("Common: Ungültige Antwort vom Gerät!");
    }

    progress.setProgress(80);
    var modulesBitfield = resp[1] + (resp[2] << 8) + (resp[3] << 16) + (resp[4] << 24);
    for (var i = 0; i < baseModuleIdPrefix.length - 1; i++) {
        var moduleActive = ((modulesBitfield >> i) & 1) == 0;
        var paramName = "BASE_ModuleEnabled_" + baseModuleIdPrefix[i + 1];
        var parModuleId = device.getParameterByName(paramName);
        if (parModuleId) 
            if (sync)
                parModuleId.value = moduleActive;
            else if (!moduleActive)
                parModuleId.value = 0;
    }

    progress.setProgress(100);
    if (sync)
        progress.setText("Common: Unterstützte Module wurden abgeglichen.");
    else
        progress.setText("Common: Nicht unterstützte Module wurden ausgeblendet.");
}

function BASE_invokeFunctionPropertyWrapper(objectIndex, propertyId, data, device, online, progress, progress_start, progress_end) {
    var apduLength = 15;
    if (typeof online.getMaxApduLength == "function") {
        apduLength = online.getMaxApduLength();
    }
    info("BASE_invokeFunctionPropertyWrapper: APDU = " + apduLength);

    // set optional parameter values
    progress_start = progress_start || 0;
    progress_end = progress_end || 0;
    
    // derived values for easier understandable calculations
    var effectiveApduLength = apduLength - 1;
    var useProgressBar = progress_end > progress_start;
    var progress_interval = progress_end - progress_start + 1;
    var progress_half = progress_start + Math.ceil(progress_interval / 2);
    var dataLength = data.length;
    // calculate number of packages (1 Byte sequence number)
    var numPackages = Math.ceil(dataLength / (effectiveApduLength));

    // general idea:
    // 1. Send metadata like APDU, Number of Packages to receive
    // 2. Send all input data in packages according to apdu size
    // 3. Call the target functionProperty
    // 4. Receive the response data in packages according to apdu size

    // 1. Send header data (APDU length, Number of Packages)
    var header = [0, apduLength, numPackages >> 8, numPackages & 0xFF, 0];
    var resp = online.invokeFunctionProperty(0x9E, 3, header);
    if (!resp || resp.length < 1 || resp[0] != 0) {
        throw new Error("Common: Keine Antwort vom Gerät!");
    }
    if (resp.length < 2) { // error
        throw new Error("Common: Ungültige Antwort vom Gerät!");
    }
    // 2. Header accepted, we send now all data
    var sequenceNumber = 0;
    do {
        // positive sequence numbers mean "send data to device"
        // response contains always the next sequence number to send
        // to repeat a package the device sends the sequence number again
        sequenceNumber = resp[1];
        var pkg = [sequenceNumber];
        var offset = (sequenceNumber - 2) * (effectiveApduLength);
        var chunkLength = Math.min(dataLength, effectiveApduLength);
        for (var j = 0; j < chunkLength; j++) {
            pkg.push(data[j+offset]);
        }
        dataLength -= chunkLength;
        if (useProgressBar) {
            // output progress bar only if requested by caller
            progress.setProgress(Math.min(progress_start + (progress_end - progress_start) / numPackages * (sequenceNumber - 1), progress_end));
        }
        progress.setText("Übertrage Sequenz " + (sequenceNumber - 1) + "/" + numPackages);
        resp = online.invokeFunctionProperty(0x9E, 3, pkg);       
    } while (resp[0] == 0 && resp.length == 2 && resp[1] > 0 && resp[1] < 128 );
    // in case of any error during send of data
    if (resp[0] == 1) {
        throw new Error("Common: Fehler beim Senden von Daten");
    }
    // 3. Data sent, call target function property
    header = [1, objectIndex, propertyId, 0];
    resp = online.invokeFunctionProperty(0x9E, 3, header);
    if (!resp || resp.length < 1 || resp[0] != 0) {
        throw new Error("Common: Keine Antwort vom Gerät beim Datenempfang!");
    }
    if (resp.length < 4) { // error
        throw new Error("Common: Ungültige Antwort nach Funktionsaufruf");
    }
    // 4. we prepare data receive
    // response contains result length...
    dataLength = (resp[1] << 8) + resp[2];
    // ... and the next (negative) sequence number
    // negative sequence numbers mean "receive data from device"
    // they are handles as abs(sequenceNumber)
    sequenceNumber = (resp[3] << 24) >> 24;  // Sign-extend via bit shifts
    numPackages = Math.ceil(dataLength / effectiveApduLength) + 1;
    var response = [];
    pkg = 1;
    do {
        // request data with a (negative) sequence number
        data = [sequenceNumber & 0xFF, 0];
        progress.setText("Empfange Sequenz " + pkg++ + "/" + numPackages);
        resp = online.invokeFunctionProperty(0x9E, 3, data);
        // device responds with same (negative) sequence number
        var respSequenceNumber = (resp[0] << 24) >> 24;  // Sign-extend via bit shifts
        if (respSequenceNumber == sequenceNumber) {
            // as long as device returns the same sequence number we copy the packet in our response
            for (var j = 1; j < resp.length; j++) {
                response.push(resp[j]);
            }
            // ... and decrease the sequence number
            sequenceNumber--;
        }
    // until the response sequence number is non-negative
    } while (respSequenceNumber < 0);
    info("BASE_invokeFunctionPropertyWrapper: response = " + response);
    return response;
}

