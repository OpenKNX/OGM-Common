
function newline(device, online, progress, context) {
    // input contains c++ like control chars (\n,\\)
    var text = device.getParameterByName(context.textbox);
    var replaced = text.value.split("\\n").join("\n");
    text.value = replaced;
}

// TODO check split in separate files, to include requried functions only

// helper-functions to delay script execution, e.g. waiting in device-communication
function openKnxWait_ms(millis) {
    var start = new Date();
    var count = 0;
    // busy waiting, as there is no other known possibility in ETS
    while (new Date() - start < millis) {
        count++;
    }
    return count;
}
function openKnxWait_s(seconds) {
    return openKnxWait_ms(seconds * 1000);
}
