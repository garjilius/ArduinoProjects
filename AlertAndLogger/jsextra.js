//Months offset by 1 because it's normally 0-11
function getTime() {
	var today = new Date();
	var dateTime = toTwoDigits(today.getDay())+'-'+toTwoDigits(today.getDate())+'-'+toTwoDigits(today.getMonth()+1)+'-'+today.getFullYear()+'+'+toTwoDigits(today.getHours())+':'+toTwoDigits(today.getMinutes())+':'+toTwoDigits(today.getSeconds());
	//Ricarico la pagina passando la stringa formattata in get, così da recuperarla con arduino
	window.location.href = '?date='+dateTime;
}

function getManualTime() {
	var input = document.getElementById('dateInput');
	var stringa = input.value;
	//DD-MM-YYYY HH:MM:SS
	var day = parseInt(stringa.substring(0, 2));
	var month = parseInt(stringa.substring(3, 5));
	var year = parseInt(stringa.substring(6, 10));
	var hours = parseInt(stringa.substring(11,13));
	var minutes = parseInt(stringa.substring(14,16));
	var seconds = parseInt(stringa.substring(15,17));
	
	var today = new Date(year, month, day, hours, minutes, seconds, 0)
		
	dateTime = toTwoDigits(today.getDay())+'-'+toTwoDigits(today.getDate())+'-'+toTwoDigits(today.getMonth())+'-'+today.getFullYear()+'+'+toTwoDigits(today.getHours())+':'+toTwoDigits(today.getMinutes())+':'+toTwoDigits(today.getSeconds());	
	window.location.href = '?date='+dateTime;
}

//In modo che se sono le 8, le segna come 08. Così posso splittare la stringa in posti fissi
function toTwoDigits(elemento) {
	var el = elemento.toString();
	if(el.length<2) {
		el = "0"+el;
	}
	return el;
}

window.onload = function() {
	var body = document.getElementById('mainBody');
	var div = document.getElementById('leg');
	var time = document.getElementById('time');
	mainBody.innerHTML = "<H1><i class=\"fas fa-cogs\"></i> Arduino Control Panel <i class=\"fas fa-cogs\"></i></H1><hr/><H2><i class=\"fas fa-user-tie\"></i> Welcome, Emanuele <i class=\"fas fa-user-tie\"></i></H2><br/><img src=\"https://dl.dropbox.com/s/xuj9q90zsbdyl2n/LogoUnisa.png?dl=0\" style=\"width:200px;height:200px;\"><br/><br/><br/><a href=\"/?deleteSD\">Delete SD Logs</a><a href=\"/?reset\">Delete Google Sheets Logs</a><a href=\"/?recovery\">Recovery</a><br/><br/><a href=\"/?logNow\">Log Now!</a><a href=\"/?sendReport\">Send Report!</a><br/><br/><a href=\"/?\">Reload Page</a><br/><br/><br/>";
	time.innerHTML = "<i class=\"far fa-clock\"></i><input placeholder=\"DD-MM-YYYY HH:MM:SS\" type=\"text\" id=\"dateInput\" size=25 name=\"date\"><button class=\"button button2\"onclick=\"getManualTime()\">Set Time Manually</button><button class=\"button button2\"onclick=\"getTime()\">Set Time Automatically</button>";
	div.innerHTML = "<br><br><b><i class=\"fas fa-sd-card\"></i> Delete SD Logs:</b> deletes log files from the SD Card<br/><b><i class=\"fas fa-table\"></i> Delete Google Sheets Logs:</b> deletes logs from Google Sheets<br/><b><i class=\"fas fa-sync-alt\"></i> Recovery:</b> Syncs data that has been logged when offline to Google Sheets<br/><b><i class=\"fas fa-file-import\"></i> Log Now:</b> Logs last sensor data to Google Sheets<br/><b><i class=\"fas fa-paper-plane\"></i> Send Report:</b> Sends minimum and maximum values for temperature and humidity of the current day via mail<br><b><i class=\"far fa-clock\"></i> Set Time:</b> Uses Client's date or manual string to set Arduino's RTC Clock";
}
