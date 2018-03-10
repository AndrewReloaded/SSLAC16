var _ip = "";
_URL = "https://raw.githubusercontent.com/bbasil2012/SSLAC16/main/latest/";
_HTML = "data/index.txt";
_FirmWare = "bin/4mb/version";
var _prg = 0;
var group;

function sv() {
    set_rest("save=", _ip)
}

function sr() {
    set_rest("reboot=", _ip)
}

function get_rest(b, d) {
    var c;
    if (window.XMLHttpRequest) {
        var a = new XMLHttpRequest()
    } else {
        var a = new ActiveXObject("Microsoft.XMLHTTP")
    }
    a.onreadystatechange = function() {
        if (a.readyState == 4 && a.status == 200) {
            c = JSON.parse(a.responseText);
            return c
        }
    };
    if ((d == undefined) || (d == "")) {
        a.open("GET", "/get?" + b, false)
    } else {
        a.open("GET", "http://" + d + "/get?" + b, false)
    }
    a.send();
    return c
}

function set_rest(b, c) {
    if (window.XMLHttpRequest) {
        var a = new XMLHttpRequest()
    } else {
        var a = new ActiveXObject("Microsoft.XMLHTTP")
    }
    if ((c == undefined) || (c == "")) {
        a.open("GET", "/set?" + b, false)
    } else {
        a.open("GET", "http://" + c + "/set?" + b, false)
    }
    a.send()
}

function post_rest(b, c) {
    if (window.XMLHttpRequest) {
        var a = new XMLHttpRequest()
    } else {
        var a = new ActiveXObject("Microsoft.XMLHTTP")
    }
    if ((c == undefined) || (c == "")) {
        a.open("POST", "/set?", false)
    } else {
        a.open("POST", "http://" + c + "/set?", false)
    }
    a.send(b)
}

function byteToHex(a) {
    var c = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"];
    return c[(a >> 4) & 15] + c[a & 15]
}

function update_progress(b) {
    if (b.lengthComputable) {
        var a = Math.round((b.loaded / b.total) * 100);
        document.getElementById(_pID).value = a
    } else {
        _prg++;
        document.getElementById(_pID).value = _pgr * 10
    }
}

function get_version() {
    return get_rest("version")
}

function uFirmWare(a) {
    var b = get_version_github(_URL, _FirmWare);
    updater(_URL + "bin/4mb/", b[0], a);
    alert("FirmWare updated. Rebooting.")
}

function uHTML(b) {
    var c = get_version_github(_URL, _HTML);
    document.getElementById(b).value = 0;
    for (i = 0; i < Object.keys(c).length; i++) {
        updater(_URL + "data/", c[i], b);
        var a = Math.round(((i + 1) / Object.keys(c).length) * 100);
        document.getElementById(b).value = a
    }
    alert("User interface updated")
}

function get_version_github(a, d) {
    var c;
    if (window.XMLHttpRequest) {
        var b = new XMLHttpRequest()
    } else {
        var b = new ActiveXObject("Microsoft.XMLHTTP")
    }
    b.onreadystatechange = function() {
        if (b.readyState == 4 && b.status == 200) {
            c = JSON.parse(b.responseText)
        }
    };
    b.open("GET", a + d, false);
    b.send();
    return c
}

function updater(b, d, c) {
    sURL = "/upload";
    _pID = c;
    window.URL = window.URL || window.webkitURL;
    var a = new XMLHttpRequest();
    a.open("GET", b + d, true);
    a.responseType = "blob";
    a.onprogress = update_progress;
    a.onload = function(h) {
        if (this.readyState == 4 && this.status == 200) {
            var f = this.response;
            var g = new FormData();
            g.append("afile", f, d);
            var j = new XMLHttpRequest();
            j.open("POST", sURL, false);
            j.send(g)
        }
    };
    a.send()
}

function toR(a) {
    return parseInt((cutHex(a)).substring(0, 2), 16)
}

function toG(a) {
    return parseInt((cutHex(a)).substring(2, 4), 16)
}

function toB(a) {
    return parseInt((cutHex(a)).substring(4, 6), 16)
}

function cutHex(a) {
    return (a.charAt(0) == "#") ? a.substring(1, 7) : a
}

function RGBtoHEX(d, c, a) {
    return "#" + ((a | c << 8 | d << 16) | 1 << 24).toString(16).slice(1)
}

function home() {
    if (_ip == "") {
        window.open("/", "_self")
    } else {
        window.open("root_page.html", "_self")
    }
};