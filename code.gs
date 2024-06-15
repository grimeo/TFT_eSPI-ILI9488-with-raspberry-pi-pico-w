
var sheet_id = '1fl_WOKciPUOZue1BBNnAGK7Ha4st393-DYCO9HWq_PQ';
var sheet_name = "Peecheck_sheet";  

function doGet(e) { 
  Logger.log(JSON.stringify(e));
  var result = 'Ok';
  if (e.parameter == 'undefined') {
    result = 'No Parameters';
  }
  else {
    
    var sheet_open = SpreadsheetApp.openById(sheet_id);
    var sheet_target = sheet_open.getSheetByName(sheet_name);

    var newRow = sheet_target.getLastRow() + 1;

    var rowDataLog = [];

    var Curr_Date = Utilities.formatDate(new Date(), "Asia/Taipei", 'dd/MM/yyyy');
    rowDataLog[0] = Curr_Date;  
    var Curr_Time = Utilities.formatDate(new Date(), "Asia/Taipei", 'HH:mm:ss');
    rowDataLog[1] = Curr_Time; 

    var sts_val = '';

    for (var param in e.parameter) {
      Logger.log('In for loop, param=' + param);
      var value = stripQuotes(e.parameter[param]);
      Logger.log(param + ':' + e.parameter[param]);
      switch (param) {
        case 'sts':
          sts_val = value;
          break;

        case 'Name':
          rowDataLog[2] = value;  
          result += ', Name Written on column C';
          break;

        case 'Age':
          rowDataLog[3] = value;  
          result += ', Age Written on column D';
          break;

        case 'Gend':
          rowDataLog[4] = value; 
          result += ', Gender on column E';
          break;

        case 'MNum':
          rowDataLog[5] = value;  
          result += ', MobileNumber Written on column F';
          break;

        case 'Uro':
          rowDataLog[6] = value;  
          result += ', Urobilinogen Written on column G';
          break;  
        
        case 'Bil':
          rowDataLog[7] = value;  
          result += ', Bilirubin Written on column H';
          break;
        
        case 'Ket':
          rowDataLog[8] = value;  
          result += ', Ketone Written on column I';
          break;
        
        case 'Cre':
          rowDataLog[9] = value;  
          result += ', Creatinine Written on column J';
          break;

        case 'Blo':
          rowDataLog[10] = value;  
          result += ', Blood Written on column K';
          break;

        case 'Pro':
          rowDataLog[11] = value;  
          result += ', Protein Written on column L';
          break;

        case 'Mic':
          rowDataLog[12] = value;  
          result += ', MicroAlbumin Written on column M';
          break;

        case 'Nit':
          rowDataLog[13] = value;  
          result += ', Nitrite Written on column N';
          break;

        case 'Leu':
          rowDataLog[14] = value;  
          result += ', Leukocytes Written on column O';
          break;

        case 'Glu':
          rowDataLog[15] = value;  
          result += ', Glucose Written on column P';
          break;

        case 'Spe':
          rowDataLog[16] = value;  
          result += ', SpecificGravity Written on column Q';
          break;

        case 'PH':
          rowDataLog[17] = value;  
          result += ', PH Written on column R';
          break;

        case 'Asc':
          rowDataLog[18] = value;  
          result += ', Ascorbate Written on column S';
          break;

        case 'Cal':
          rowDataLog[19] = value;  
          // result += ', Calcium Written on column T';
          result = 'Success';
          break;

        default:
          result += ", unsupported parameter";

          Logger.log(JSON.stringify(rowDataLog));
          var newRangeDataLog = sheet_target.getRange(newRow, 1, 1, rowDataLog.length);
          newRangeDataLog.setValues([rowDataLog]);

          return ContentService.createTextOutput(result);
      }
    }
    
    if (sts_val == 'write') {
      Logger.log(JSON.stringify(rowDataLog));
      var newRangeDataLog = sheet_target.getRange(newRow, 1, 1, rowDataLog.length);
      newRangeDataLog.setValues([rowDataLog]);
      
      return ContentService.createTextOutput(result);
    }
    
    if (sts_val == 'read') {

      var all_Data = sheet_target.getRange("Peecheck_sheet!C4:T"+sheet_target.getLastRow()+"").getDisplayValues();
      // var stringifiedData = JSON.stringify(all_Data);
      return ContentService.createTextOutput(all_Data);
    }
  }
}
function stripQuotes( value ) {
  return value.replace(/^["']|['"],$/g, "");
}