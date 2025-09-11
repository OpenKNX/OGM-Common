
function newline(device, online, progress, context) {
    // input contains c++ like control chars (\n,\\)
    var text = device.getParameterByName(context.textbox);
    var replaced = text.value.split("\\n").join("\n");
    text.value = replaced;
}

function BASE_getUnsupportedEtsModules(device, online, progress, context) {
    progress.setText("Common: Frage Hardware nach unterstützten Modulen...");
    online.connect();
    progress.setProgress(50);
    
    var data = [0]; // no input data
    var resp = online.invokeFunctionProperty(158, 2, data);

    if (resp[0] != 0) { // error
        throw new Error("Common: Keine Antwort vom Gerät!");
    }
    
    online.disconnect();
    progress.setProgress(100);
    progress.setText("Common: Nicht unterstützte Module wurden ausgeblendet.");
    var modulesBitfield = resp[1] + (resp[2] << 8) + (resp[3] << 16) + (resp[4] << 24);
    for (var i = 0; i < baseModuleIdPrefix.length - 1; i++) {
        if (((modulesBitfield >> i) & 1) != 0) {
            var paramName = "BASE_ModuleEnabled_" + baseModuleIdPrefix[i + 1];
            var parModuleId = device.getParameterByName(paramName);
            if (parModuleId) 
                parModuleId.value = 0;
        }
    }
}
