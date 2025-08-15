
function newline(device, online, progress, context) {
    // input contains c++ like control chars (\n,\\)
    var text = device.getParameterByName(context.textbox);
    var replaced = text.value.split("\\n").join("\n");
    text.value = replaced;
}


/**
 * Wrapper for invoking function properties for this module
 * @param {object} online - The online connection object
 * @param {number[]} request - Request data array
 * @returns {number[]} Response data array
 */
function BASE_invokeFunctionProperty(online, request) {
    var baseFunctionId = 160;
    var basePropertyId = 8;
    return online.invokeFunctionProperty(baseFunctionId, basePropertyId, request);
}

function BASE_funcPropInfo(device, online, progress, context) {
    progress.setProgress(1);

    progress.setText("Verbinde mit OpenKNX-Geräte...");
    online.connect();
    progress.setProgress(10);

    progress.setText("Request Vorbereiten...");
    var request = device.getParameterByName("BASE_FuncPropIn").value.split(",");
    for (var i = 0; i < request.length; i++) {
        request[i] = parseInt(request[i], 10);
    }
    progress.setProgress(20);

    progress.setText("Datenabruf von Gerät...");
    var response = BASE_invokeFunctionProperty(online, request);
    progress.setProgress(80);

    online.disconnect();
    progress.setProgress(90);

    device.getParameterByName("BASE_FuncPropOut").value = response.join(",");
    progress.setProgress(100);

    progress.setText("Datenabruf von Gerät [OK]");
}

function BASE_funcPropKOs(device, online, progress, context) {
    progress.setProgress(1);

    progress.setText("Verbinde mit OpenKNX-Geräte...");
    online.connect();
    progress.setProgress(10);

    progress.setText("Request Vorbereiten...");
    var request = device.getParameterByName("BASE_FuncPropIn").value.split(",");
    for (var i = 0; i < request.length; i++) {
        request[i] = parseInt(request[i], 10);
    }
    var koMin = request.length > 0 ? request[0] : 1;
    var koMax = request.length > 2 ? request[1] : 19;

    var output = [];
    for (var i = koMin; i <= koMax; i++) {
        progress.setText("Prüfe KO " + i + "...");
        progress.setProgress(10 + (i - koMin) * 80 / (koMax - koMin));
    
        var response = BASE_invokeFunctionProperty(online, [0x10, 0x00, ((i >> 8) & 0xff), (i & 0xff) ]);
        if (response[0] !== 0x00) {
            output.push("KO " + i + ": Fehler");
        }
        else {
            var flags = response[1];
            output.push("KO " + i + ": \t" 
                + ((flags & 0x80) ? "K":"-" )
                + ((flags & 0x40) ? "L":"-" )
                + ((flags & 0x20) ? "S":"-" )
                + ((flags & 0x10) ? "Ü":"-" )
                + ((flags & 0x08) ? "A":"-" )
                + ((flags & 0x04) ? "I":"-" )
                + ((flags & 0x03)) // prio-bits
            );
        }
    }


    online.disconnect();
    progress.setProgress(90);

    device.getParameterByName("BASE_FuncPropOut").value = output.join("\n");
    progress.setProgress(100);

    progress.setText("KO-Info [OK]");
}
