
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
