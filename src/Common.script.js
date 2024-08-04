
function newline(device, online, progress, context) {
    // input contains c++ like control chars (\n,\\)
    var text = device.getParameterByName(context.textbox);
    var replaced = text.value.split("\\n").join("\n");
    text.value = replaced;
}

function checkAPDU(device, online, progress, context) {
    progress.setText("Ermittle APDU-Länge...");
    online.connect();

    var high = 251;
    var low = 12;
    var diffApdu = 3;

    while (low < high) {
        var data = [];
        var mid = Math.floor((low + high + 1) / 2);

        progress.setText("Versuche APDU-Länge " + (mid + diffApdu));

        for (var i = 0; i < mid; i++)
            data.push(parseInt(mid.toString(16), 16));


        try {
            var resp = online.invokeFunctionProperty(0x9E, 0, data);
            if (resp[0] != mid) throw new Error("");

            low = mid;
        } catch (e) {
            high = mid - 1;
        }
    }

    device.getParameterByName("BASE_APDU").value = low + diffApdu;

    online.disconnect();
    progress.setText("Ermittlung abgeschlossen.");
}
