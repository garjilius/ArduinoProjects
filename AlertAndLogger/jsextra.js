
//Months offset by 1 because it's normally 0-11, seconds offset by -5 to account for arduino's delays
function getTime() {
	var today = new Date();
	dateTime = toTwoDigits(today.getDay())+'-'+toTwoDigits(today.getDate())+'-'+toTwoDigits(today.getMonth()+1)+'-'+today.getFullYear()+'+'+toTwoDigits(today.getHours())+':'+toTwoDigits(today.getMinutes())+':'+toTwoDigits(today.getSeconds()-5);
	//Ricarico la pagina passando la stringa formattata in get, così da recuperarla con arduino
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
	mainBody.innerHTML = "<H1><i class=\"fas fa-cogs\"></i> Arduino Control Panel <i class=\"fas fa-cogs\"></i></H1><hr/><H2><i class=\"fas fa-user-tie\"></i> Welcome, Emanuele <i class=\"fas fa-user-tie\"></i></H2><br/><img src=\"https://dl.dropbox.com/s/xuj9q90zsbdyl2n/LogoUnisa.png?dl=0\" style=\"width:200px;height:200px;\"><br/><br/><br/><a href=\"/?deleteSD\">Delete SD Logs</a><a href=\"/?reset\">Delete Google Sheets Logs</a><a href=\"/?recovery\">Recovery</a><br/><br/><a href=\"/?logNow\">Log Now!</a><a href=\"/?sendReport\">Send Report!</a><br/><br/><a href=\"/?\">Reload Page</a><br/><br/><button class=\"button button2\"onclick=\"getTime()\">Set Time</button><br/><br/>";
	div.innerHTML = "<br><br><b><i class=\"fas fa-sd-card\"></i> Delete SD Logs:</b> deletes log files from the SD Card<br/><b><i class=\"fas fa-table\"></i> Delete Google Sheets Logs:</b> deletes logs from Google Sheets<br/><b><i class=\"fas fa-sync-alt\"></i> Recovery:</b> Syncs data that has been logged when offline to Google Sheets<br/><b><i class=\"fas fa-file-import\"></i> Log Now:</b> Logs last sensor data to Google Sheets<br/><b><i class=\"fas fa-paper-plane\"></i> Send Report:</b> Sends minimum and maximum values for temperature and humidity of the current day via mail<br><b><i class=\"far fa-clock\"></i> Set Time:</b> Uses Client data to set Arduino's RTC Clock";
}
