/**
 * wm_strings_en.h
 * engligh strings for
 * WiFiManager, a library for the ESP8266/Arduino platform
 * for configuration of WiFi credentials using a Captive Portal
 *
 * @author Creator tzapu
 * @author tablatronix
 * @version 0.0.0
 * @license MIT
 */

#ifndef _WM_STRINGS_EN_H_
#define _WM_STRINGS_EN_H_


#ifndef WIFI_MANAGER_OVERRIDE_STRINGS
// !!! ABOVE WILL NOT WORK if you define in your sketch, must be build flag, if anyone one knows how to order includes to be able to do this it would be neat.. I have seen it done..

// strings files must include a consts file!
#include "wm_consts_en.h" // include constants, tokens, routes

const char WM_LANGUAGE[] PROGMEM = "en-US"; // i18n lang code

const char HTTP_HEAD_START[]       PROGMEM = "<!DOCTYPE html>"
"<html lang='en'><head>"
"<meta name='format-detection' content='telephone=no'>"
"<meta charset='UTF-8'>"
"<meta  name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>"
"<title>{v}</title>";

const char HTTP_SCRIPT[]           PROGMEM = "<script>function c(l){"
"document.getElementById('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;"
"p = l.nextElementSibling.classList.contains('l');"
"document.getElementById('p').disabled = !p;"
"if(p)document.getElementById('p').focus();};"
"function f() {var x = document.getElementById('p');x.type==='password'?x.type='text':x.type='password';}"
"</script>"; // @todo add button states, disable on click , show ack , spinner etc

const char HTTP_HEAD_END[]         PROGMEM = "</head><body class='{c}'><div class='wrap'>"; // {c} = _bodyclass
// example of embedded logo, base64 encoded inline, No styling here
// const char HTTP_ROOT_MAIN[]        PROGMEM = "<img title=' alt=' src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAADQElEQVRoQ+2YjW0VQQyE7Q6gAkgFkAogFUAqgFQAVACpAKiAUAFQAaECQgWECggVGH1PPrRvn3dv9/YkFOksoUhhfzwz9ngvKrc89JbnLxuA/63gpsCmwCADWwkNEji8fVNgotDM7osI/x777x5l9F6JyB8R4eeVql4P0y8yNsjM7KGIPBORp558T04A+CwiH1UVUItiUQmZ2XMReSEiAFgjAPBeVS96D+sCYGaUx4cFbLfmhSpnqnrZuqEJgJnd8cQplVLciAgX//Cf0ToIeOB9wpmloLQAwpnVmAXgdf6pwjpJIz+XNoeZQQZlODV9vhc1Tuf6owrAk/8qIhFbJH7eI3eEzsvydQEICqBEkZwiALfF70HyHPpqScPV5HFjeFu476SkRA0AzOfy4hYwstj2ZkDgaphE7m6XqnoS7Q0BOPs/sw0kDROzjdXcCMFCNwzIy0EcRcOvBACfh4k0wgOmBX4xjfmk4DKTS31hgNWIKBCI8gdzogTgjYjQWFMw+o9LzJoZ63GUmjWm2wGDc7EvDDOj/1IVMIyD9SUAL0WEhpriRlXv5je5S+U1i2N88zdPuoVkeB+ls4SyxCoP3kVm9jsjpEsBLoOBNC5U9SwpGdakFkviuFP1keblATkTENTYcxkzgxTKOI3jyDxqLkQT87pMA++H3XvJBYtsNbBN6vuXq5S737WqHkW1VgMQNXJ0RshMqbbT33sJ5kpHWymzcJjNTeJIymJZtSQd9NHQHS1vodoFoTMkfbJzpRnLzB2vi6BZAJxWaCr+62BC+jzAxVJb3dmmiLzLwZhZNPE5e880Suo2AZgB8e8idxherqUPnT3brBDTlPxO3Z66rVwIwySXugdNd+5ejhqp/+NmgIwGX3Py3QBmlEi54KlwmjkOytQ+iJrLJj23S4GkOeecg8G091no737qvRRdzE+HLALQoMTBbJgBsCj5RSWUlUVJiZ4SOljb05eLFWgoJ5oY6yTyJp62D39jDANoKKcSocPJD5dQYzlFAFZJflUArgTPZKZwLXAnHmerfJquUkKZEgyzqOb5TuDt1P3nwxobqwPocZA11m4A1mBx5IxNgRH21ti7KbAGiyNn3HoF/gJ0w05A8xclpwAAAABJRU5ErkJggg==' /><h1>{v}</h1><h3>WiFiManager</h3>";
const char HTTP_ROOT_MAIN[]        PROGMEM = "<h1>{t}</h1><h3>Access Point SSID: {v}<br>Access Point Password: {i}</h3>";

const char * const HTTP_PORTAL_MENU[] PROGMEM = {
"<form action='/wifi'    method='get'><button>Configure Client WiFi</button></form><br/>\n", // MENU_WIFI
"<form action='/0wifi'   method='get'><button>Configure WiFi (No scan)</button></form><br/>\n", // MENU_WIFINOSCAN
"<form action='/info'    method='get'><button>Info</button></form><br/>\n", // MENU_INFO
"<form action='/param'   method='get'><button>Configure Controller</button></form><br/>\n",//MENU_PARAM
"<form action='/close'   method='get'><button>Close</button></form><br/>\n", // MENU_CLOSE
"<form action='/restart' method='get'><button>Restart</button></form><br/>\n",// MENU_RESTART
"<form action='/exit'    method='get'><button>Exit</button></form><br/>\n",  // MENU_EXIT
"<form action='/erase'   method='get'><button class='D'>Erase</button></form><br/>\n", // MENU_ERASE
"<form action='/update'  method='get'><button style='background-color: #c26402;'>Update</button></form><br/>\n",// MENU_UPDATE
"<hr><br/>" // MENU_SEP
};

// const char HTTP_PORTAL_OPTIONS[]   PROGMEM = strcat(HTTP_PORTAL_MENU[0] , HTTP_PORTAL_MENU[3] , HTTP_PORTAL_MENU[7]);
const char HTTP_PORTAL_OPTIONS[]   PROGMEM = "";
const char HTTP_ITEM_QI[]          PROGMEM = "<div role='img' aria-label='{r}%' title='{r}%' class='q q-{q} {i} {h}'></div>"; // rssi icons
const char HTTP_ITEM_QP[]          PROGMEM = "<div class='q {h}'>{r}%</div>"; // rssi percentage {h} = hidden showperc pref
const char HTTP_ITEM[]             PROGMEM = "<div><a href='#p' onclick='c(this)' data-ssid='{V}'>{v}</a>{qi}{qp}</div>"; // {q} = HTTP_ITEM_QI, {r} = HTTP_ITEM_QP
// const char HTTP_ITEM[]            PROGMEM = "<div><a href='#p' onclick='c(this)'>{v}</a> {R} {r}% {q} {e}</div>"; // test all tokens

const char HTTP_FORM_START[]       PROGMEM = "<form method='POST' action='{v}'>";
const char HTTP_FORM_WIFI[]        PROGMEM = "<label for='s'>SSID</label><input id='s' name='s' maxlength='32' autocorrect='off' autocapitalize='none' placeholder='{v}'><br/><label for='p'>Password</label><input id='p' name='p' maxlength='64' type='password' placeholder='{p}'><input type='checkbox' onclick='f()'> Show Password";
const char HTTP_FORM_WIFI_END[]    PROGMEM = "";
const char HTTP_FORM_STATIC_HEAD[] PROGMEM = "<hr><br/>";
const char HTTP_FORM_END[]         PROGMEM = "<br/><br/><button type='submit'>Save</button></form>";
const char HTTP_FORM_LABEL[]       PROGMEM = "<label for='{i}'>{t}</label>";
const char HTTP_FORM_PARAM_HEAD[]  PROGMEM = "<hr><br/>";
const char HTTP_FORM_PARAM[]       PROGMEM = "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n"; // do not remove newline!

const char HTTP_SCAN_LINK[]        PROGMEM = "<br/><form action='/wifi?refresh=1' method='POST'><button name='refresh' value='1'>Refresh</button></form>";
const char HTTP_SAVED[]            PROGMEM = "<div class='msg'>Saving Credentials<br/>Trying to connect ESP to network.<br />If it fails reconnect to AP to try again</div>";
const char HTTP_PARAMSAVED[]       PROGMEM = "<div class='msg S'>Saved<br/></div>";
const char HTTP_END[]              PROGMEM = "</div></body></html>";
const char HTTP_ERASEBTN[]         PROGMEM = "<br/><form action='/erase' method='get'><button class='D'>Erase WiFi config</button></form>";
const char HTTP_UPDATEBTN[]        PROGMEM = "<br/><form action='/update' method='get'><button>Update</button></form>";
const char HTTP_BACKBTN[]          PROGMEM = "<hr><br/><form action='/' method='get'><button>Back</button></form>";

const char HTTP_STATUS_ON[]        PROGMEM = "<div class='msg S'><strong>Connected</strong> to {v}<br/><em><small>with IP {i}</small></em></div>";
const char HTTP_STATUS_OFF[]       PROGMEM = "<div class='msg {c}'><strong>Not connected</strong> to {v}{r}</div>"; // {c=class} {v=ssid} {r=status_off}
const char HTTP_STATUS_OFFPW[]     PROGMEM = "<br/>Authentication failure"; // STATION_WRONG_PASSWORD,  no eps32
const char HTTP_STATUS_OFFNOAP[]   PROGMEM = "<br/>AP not found";   // WL_NO_SSID_AVAIL
const char HTTP_STATUS_OFFFAIL[]   PROGMEM = "<br/>Could not connect"; // WL_CONNECT_FAILED
const char HTTP_STATUS_NONE[]      PROGMEM = "<div class='msg'>No Client WiFi network set</div>";
const char HTTP_BR[]               PROGMEM = "<br/>";

const char HTTP_STYLE[]            PROGMEM = "<style>"
".c,body{text-align:center;font-family:verdana; background-color:#060606; color:#fff; background-repeat: no-repeat;background-attachment: fixed;background-position: center;background-size: 85vh;background-image: url(data:image/png;base64,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);}div,input,select{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box}"
"input,button,select,.msg{border-radius:.3rem;width: 100%}input[type=radio],input[type=checkbox]{width:auto}"
"button,input[type='button'],input[type='submit']{cursor:pointer;border:0;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%}"
"input[type='file']{border:1px solid #1fa3ec}"
".wrap {text-align:left;display:inline-block;min-width:260px;max-width:500px}"
// links
"a{color:#000;font-weight:700;text-decoration:none}a:hover{color:#1fa3ec;text-decoration:underline}"
// quality icons
".q{height:16px;margin:0;padding:0 5px;text-align:right;min-width:38px;float:right}.q.q-0:after{background-position-x:0}.q.q-1:after{background-position-x:-16px}.q.q-2:after{background-position-x:-32px}.q.q-3:after{background-position-x:-48px}.q.q-4:after{background-position-x:-64px}.q.l:before{background-position-x:-80px;padding-right:5px}.ql .q{float:left}.q:after,.q:before{content:'';width:16px;height:16px;display:inline-block;background-repeat:no-repeat;background-position: 16px 0;"
"background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAAAQCAMAAADeZIrLAAAAJFBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHJj5lAAAAC3RSTlMAIjN3iJmqu8zd7vF8pzcAAABsSURBVHja7Y1BCsAwCASNSVo3/v+/BUEiXnIoXkoX5jAQMxTHzK9cVSnvDxwD8bFx8PhZ9q8FmghXBhqA1faxk92PsxvRc2CCCFdhQCbRkLoAQ3q/wWUBqG35ZxtVzW4Ed6LngPyBU2CobdIDQ5oPWI5nCUwAAAAASUVORK5CYII=');}"
// icons @2x media query (32px rescaled)
"@media (-webkit-min-device-pixel-ratio: 2),(min-resolution: 192dpi){.q:before,.q:after {"
"background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALwAAAAgCAMAAACfM+KhAAAALVBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAOrOgAAAADnRSTlMAESIzRGZ3iJmqu8zd7gKjCLQAAACmSURBVHgB7dDBCoMwEEXRmKlVY3L//3NLhyzqIqSUggy8uxnhCR5Mo8xLt+14aZ7wwgsvvPA/ofv9+44334UXXngvb6XsFhO/VoC2RsSv9J7x8BnYLW+AjT56ud/uePMdb7IP8Bsc/e7h8Cfk912ghsNXWPpDC4hvN+D1560A1QPORyh84VKLjjdvfPFm++i9EWq0348XXnjhhT+4dIbCW+WjZim9AKk4UZMnnCEuAAAAAElFTkSuQmCC');"
"background-size: 95px 16px;}}"
// msg callouts
".msg{padding:20px;margin:20px 0;border:1px solid #eee;border-left-width:5px;border-left-color:#777}.msg h4{margin-top:0;margin-bottom:5px}.msg.P{border-left-color:#1fa3ec}.msg.P h4{color:#1fa3ec}.msg.D{border-left-color:#dc3630}.msg.D h4{color:#dc3630}.msg.S{border-left-color: #5cb85c}.msg.S h4{color: #5cb85c}"
// lists
"dt{font-weight:bold}dd{margin:0;padding:0 0 0.5em 0;min-height:12px}"
"td{vertical-align: top;}"
".h{display:none}"
"button{transition: 0s opacity;transition-delay: 3s;transition-duration: 0s;cursor: pointer}"
"button.D{background-color:#dc3630}"
"button:active{opacity:50% !important;cursor:wait;transition-delay: 0s}"
// invert
"body.invert .msg{color:#fff;background-color:#282828;border-top:1px solid #555;border-right:1px solid #555;border-bottom:1px solid #555;}"
"body.invert .q[role=img]{-webkit-filter:invert(1);filter:invert(1);}"
":disabled {opacity: 0.5;}"
"</style>";

#ifndef WM_NOHELP
const char HTTP_HELP[]             PROGMEM =
 "<br/><h3>Available pages</h3><hr>"
 "<table class='table'>"
 "<thead><tr><th>Page</th><th>Function</th></tr></thead><tbody>"
 "<tr><td><a href='/'>/</a></td>"
 "<td>Menu page.</td></tr>"
 "<tr><td><a href='/wifi'>/wifi</a></td>"
 "<td>Show WiFi scan results and enter WiFi configuration.(/0wifi noscan)</td></tr>"
 "<tr><td><a href='/wifisave'>/wifisave</a></td>"
 "<td>Save WiFi configuration information and configure device. Needs variables supplied.</td></tr>"
 "<tr><td><a href='/param'>/param</a></td>"
 "<td>Parameter page</td></tr>"
 "<tr><td><a href='/info'>/info</a></td>"
 "<td>Information page</td></tr>"
 "<tr><td><a href='/u'>/u</a></td>"
 "<td>OTA Update</td></tr>"
 "<tr><td><a href='/close'>/close</a></td>"
 "<td>Close the captiveportal popup, config portal will remain active</td></tr>"
 "<tr><td>/exit</td>"
 "<td>Exit Config portal, config portal will close</td></tr>"
 "<tr><td>/restart</td>"
 "<td>Reboot the device</td></tr>"
 "<tr><td>/erase</td>"
 "<td>Erase WiFi configuration and reboot device. Device will not reconnect to a network until new WiFi configuration data is entered.</td></tr>"
 "</table>"
 "<p/>Github <a href='https://github.com/tzapu/WiFiManager'>https://github.com/tzapu/WiFiManager</a>.";
#else
const char HTTP_HELP[]             PROGMEM = "";
#endif

const char HTTP_UPDATE[] PROGMEM = "Upload new firmware<br/><form method='POST' action='u' enctype='multipart/form-data' onchange=\"(function(el){document.getElementById('uploadbin').style.display = el.value=='' ? 'none' : 'initial';})(this)\"><input type='file' name='update' accept='.bin,application/octet-stream'><button id='uploadbin' type='submit' class='h D'>Update</button></form><small><a href='http://192.168.4.1/update' target='_blank'>* May not function inside captive portal, open in browser http://192.168.4.1</a><small>";
const char HTTP_UPDATE_FAIL[] PROGMEM = "<div class='msg D'><strong>Update failed!</strong><Br/>Reboot device and try again</div>";
const char HTTP_UPDATE_SUCCESS[] PROGMEM = "<div class='msg S'><strong>Update successful.  </strong> <br/> Device rebooting now...</div>";

#ifdef WM_JSTEST
const char HTTP_JS[] PROGMEM =
"<script>function postAjax(url, data, success) {"
"    var params = typeof data == 'string' ? data : Object.keys(data).map("
"            function(k){ return encodeURIComponent(k) + '=' + encodeURIComponent(data[k]) }"
"        ).join('&');"
"    var xhr = window.XMLHttpRequest ? new XMLHttpRequest() : new ActiveXObject(\"Microsoft.XMLHTTP\");"
"    xhr.open('POST', url);"
"    xhr.onreadystatechange = function() {"
"        if (xhr.readyState>3 && xhr.status==200) { success(xhr.responseText); }"
"    };"
"    xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');"
"    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');"
"    xhr.send(params);"
"    return xhr;}"
"postAjax('/status', 'p1=1&p2=Hello+World', function(data){ console.log(data); });"
"postAjax('/status', { p1: 1, p2: 'Hello World' }, function(data){ console.log(data); });"
"</script>";
#endif

// Info html
// @todo remove html elements from progmem, repetetive strings
#ifdef ESP32
	const char HTTP_INFO_esphead[]    PROGMEM = "<h3>esp32</h3><hr><dl>";
	const char HTTP_INFO_chiprev[]    PROGMEM = "<dt>Chip rev</dt><dd>{1}</dd>";
  	const char HTTP_INFO_lastreset[]  PROGMEM = "<dt>Last reset reason</dt><dd>CPU0: {1}<br/>CPU1: {2}</dd>";
  	const char HTTP_INFO_aphost[]     PROGMEM = "<dt>Access point hostname</dt><dd>{1}</dd>";
    const char HTTP_INFO_psrsize[]    PROGMEM = "<dt>PSRAM Size</dt><dd>{1} bytes</dd>";
	const char HTTP_INFO_temp[]       PROGMEM = "<dt>Temperature</dt><dd>{1} C&deg; / {2} F&deg;</dd>";
    const char HTTP_INFO_hall[]       PROGMEM = "<dt>Hall</dt><dd>{1}</dd>";
#else
	const char HTTP_INFO_esphead[]    PROGMEM = "<h3>esp8266</h3><hr><dl>";
	const char HTTP_INFO_fchipid[]    PROGMEM = "<dt>Flash chip ID</dt><dd>{1}</dd>";
	const char HTTP_INFO_corever[]    PROGMEM = "<dt>Core version</dt><dd>{1}</dd>";
	const char HTTP_INFO_bootver[]    PROGMEM = "<dt>Boot version</dt><dd>{1}</dd>";
	const char HTTP_INFO_lastreset[]  PROGMEM = "<dt>Last reset reason</dt><dd>{1}</dd>";
	const char HTTP_INFO_flashsize[]  PROGMEM = "<dt>Real flash size</dt><dd>{1} bytes</dd>";
#endif

const char HTTP_INFO_memsmeter[]  PROGMEM = "<br/><progress value='{1}' max='{2}'></progress></dd>";
const char HTTP_INFO_memsketch[]  PROGMEM = "<dt>Memory - Sketch size</dt><dd>Used / Total bytes<br/>{1} / {2}";
const char HTTP_INFO_freeheap[]   PROGMEM = "<dt>Memory - Free heap</dt><dd>{1} bytes available</dd>";
const char HTTP_INFO_wifihead[]   PROGMEM = "<br/><h3>WiFi</h3><hr>";
const char HTTP_INFO_uptime[]     PROGMEM = "<dt>Uptime</dt><dd>{1} mins {2} secs</dd>";
const char HTTP_INFO_chipid[]     PROGMEM = "<dt>Chip ID</dt><dd>{1}</dd>";
const char HTTP_INFO_idesize[]    PROGMEM = "<dt>Flash size</dt><dd>{1} bytes</dd>";
const char HTTP_INFO_sdkver[]     PROGMEM = "<dt>SDK version</dt><dd>{1}</dd>";
const char HTTP_INFO_cpufreq[]    PROGMEM = "<dt>CPU frequency</dt><dd>{1}MHz</dd>";
const char HTTP_INFO_apip[]       PROGMEM = "<dt>Access point IP</dt><dd>{1}</dd>";
const char HTTP_INFO_apmac[]      PROGMEM = "<dt>Access point MAC</dt><dd>{1}</dd>";
const char HTTP_INFO_apssid[]     PROGMEM = "<dt>Access point SSID</dt><dd>{1}</dd>";
const char HTTP_INFO_apbssid[]    PROGMEM = "<dt>BSSID</dt><dd>{1}</dd>";
const char HTTP_INFO_stassid[]    PROGMEM = "<dt>Station SSID</dt><dd>{1}</dd>";
const char HTTP_INFO_staip[]      PROGMEM = "<dt>Station IP</dt><dd>{1}</dd>";
const char HTTP_INFO_stagw[]      PROGMEM = "<dt>Station gateway</dt><dd>{1}</dd>";
const char HTTP_INFO_stasub[]     PROGMEM = "<dt>Station subnet</dt><dd>{1}</dd>";
const char HTTP_INFO_dnss[]       PROGMEM = "<dt>DNS Server</dt><dd>{1}</dd>";
const char HTTP_INFO_host[]       PROGMEM = "<dt>Hostname</dt><dd>{1}</dd>";
const char HTTP_INFO_stamac[]     PROGMEM = "<dt>Station MAC</dt><dd>{1}</dd>";
const char HTTP_INFO_conx[]       PROGMEM = "<dt>Connected</dt><dd>{1}</dd>";
const char HTTP_INFO_autoconx[]   PROGMEM = "<dt>Autoconnect</dt><dd>{1}</dd>";

const char HTTP_INFO_aboutver[]     PROGMEM = "<dt>WiFiManager</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutarduino[] PROGMEM = "<dt>Arduino</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutsdk[]     PROGMEM = "<dt>ESP-SDK/IDF</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutdate[]    PROGMEM = "<dt>Build date</dt><dd>{1}</dd>";

const char S_brand[]              PROGMEM = "AE Heater Controller";
const char S_debugPrefix[]        PROGMEM = "*AE:";
const char S_y[]                  PROGMEM = "Yes";
const char S_n[]                  PROGMEM = "No";
const char S_enable[]             PROGMEM = "Enabled";
const char S_disable[]            PROGMEM = "Disabled";
const char S_GET[]                PROGMEM = "GET";
const char S_POST[]               PROGMEM = "POST";
const char S_NA[]                 PROGMEM = "Unknown";
const char S_passph[]             PROGMEM = "********";
const char S_titlewifisaved[]     PROGMEM = "Credentials saved";
const char S_titlewifisettings[]  PROGMEM = "Settings saved";
const char S_titlewifi[]          PROGMEM = "Config ESP";
const char S_titleinfo[]          PROGMEM = "Info";
const char S_titleparam[]         PROGMEM = "Setup";
const char S_titleparamsaved[]    PROGMEM = "Setup saved";
const char S_titleexit[]          PROGMEM = "Exit";
const char S_titlereset[]         PROGMEM = "Reset";
const char S_titleerase[]         PROGMEM = "Erase";
const char S_titleclose[]         PROGMEM = "Close";
const char S_options[]            PROGMEM = "options";
const char S_nonetworks[]         PROGMEM = "No networks found. Refresh to scan again.";
const char S_staticip[]           PROGMEM = "Static IP";
const char S_staticgw[]           PROGMEM = "Static gateway";
const char S_staticdns[]          PROGMEM = "Static DNS";
const char S_subnet[]             PROGMEM = "Subnet";
const char S_exiting[]            PROGMEM = "Exiting";
const char S_resetting[]          PROGMEM = "Module will reset in a few seconds.";
const char S_closing[]            PROGMEM = "You can close the page, portal will continue to run";
const char S_error[]              PROGMEM = "An error occured";
const char S_notfound[]           PROGMEM = "File not found\n\n";
const char S_uri[]                PROGMEM = "URI: ";
const char S_method[]             PROGMEM = "\nMethod: ";
const char S_args[]               PROGMEM = "\nArguments: ";
const char S_parampre[]           PROGMEM = "param_";

// debug strings
const char D_HR[]                 PROGMEM = "--------------------";


// softap ssid default prefix
#ifdef ESP8266
    const char S_ssidpre[]        PROGMEM = "ESP";
#elif defined(ESP32)
    const char S_ssidpre[]        PROGMEM = "ESP32";
#else
    const char S_ssidpre[]        PROGMEM = "WM";
#endif

// END WIFI_MANAGER_OVERRIDE_STRINGS
#endif

#endif