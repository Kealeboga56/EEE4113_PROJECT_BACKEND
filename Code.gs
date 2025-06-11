function doPost(e) {
  //var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Sheet1");
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  try {
    Logger.log("Incoming data: " + e.postData.contents);
    
    var data = JSON.parse(e.postData.contents);

    var timestamp = new Date();
    sheet.appendRow([data.rfid,timestamp, data.weight, data.imageurl]);

    return ContentService.createTextOutput("Success");
  } catch (err) {
    Logger.log("Error: " + err.toString());
    return ContentService.createTextOutput("Error with whats happening: " + err.toString());
  }
}
