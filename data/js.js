//const { left } = require("@popperjs/core");

var timer = 0;

function decT() {
  timer--;
  document.getElementById("time").innerHTML = timer;
  if (timer == 0) {
    window.location = "/";
  } else {
    window.setTimeout("decT()", 1000);
  }
}

function setT(t) {
  timer = t;
  decT();
}

function reqChange() {
  if (this.readyState == 4) {
    if (this.status == 200 && this.responseText != null) {
      var data = this.responseText.split("&");
      var l = (data.length - 1) / 2 - 1;
      document.getElementById("now"  ).innerText = data[0];
      for (var i = 0; i <= l; i++) {
        var r = document.getElementById("r" + i);
        if(data[1 + i * 2] == 1) {
          r.innerText = "Ligado";
          r.style.background = "rgb(170, 236, 83)";
        } else {
          r.innerText = "Desligado";
          r.style.background = "rgb(227, 0, 14)";
        }
        document.getElementById("e" + i).innerText = data[2 + i * 2];
      }
    } else {
      alert("Falha obtendo status do relé.");
    }
  }
}

var reqStatus = new XMLHttpRequest();
reqStatus.onreadystatechange = reqChange;

var reqSet = new XMLHttpRequest();
reqSet.onreadystatechange = reqChange;

function relayStatus() {
  reqStatus.open("GET", "/relayStatus", true);
  reqStatus.send(null);
  window.setTimeout("relayStatus()", 2000);
}

function setRelay(s, c) {
  var x = s[s.length - 1];
  if (x == "0") {
    x = "desligar";
  } else if (x == "1") {
    x = "ligar";
  } else {
    x = "inverter";
  }
  if (c == 0 || confirm("Confirma " + x + "?")) {
    reqSet.open("GET", "/relayStatus?set=" + s, true);
    reqSet.send(null);
  }
}

function logDownload() {
  window.location = "/logGet";
}

function logReset() {
  if (confirm('Confirma reiniciar Log?')) {
    window.location = "/logReset";
  }
}

function chkReset() {
  if (confirm("Confirma redefinir todas as Configurações?")) {
    window.location = "/configReset";
  }
}

function menu(m) {
  var s = document.getElementById(m).style;
  if (s.display == "none") {
    s.display = "";
  } else {
    s.display = "none";
  }
}
   
function getData()
{
var xmlhttp = new XMLHttpRequest();
xmlhttp.onreadystatechange = function() 
{
if (xmlhttp.readyState == XMLHttpRequest.DONE && xmlhttp.status == 200)
{
  var myObj = JSON.parse(this.responseText);
  
 // for (var i = 0; i < 5; i++)
  {
 var numero1 = document.getElementById('Temp1').innerText= parseInt(myObj.Tempo.Hora);
 var numero2 = document.getElementById('Temp2').innerText= parseInt(myObj.Tempo.Temp);
 //var numero3 = document.getElementById('Temp3').innerText= (myObj.Tempo);
 //var numero4 = document.getElementById('Temp4').innerText= (myObj.Temp);
  }
//     values.push(myObj);
//     timeStamp.push(time);
//   var row = table.insertRow(1);	//Add after headings
console.log(myObj);
console.log(Temp1);
console.log(Temp2);
console.log(numero1);
console.log(numero2);

//var data = new google.visualization.arraytoDataTable//();
//data.addColumn('number', 'Dia');
//data.addColumn('number', 'Temp');
//data.addRows
//([      ['Hora', 'Temp'],
  //[Temp1,Temp2],
 // ['Temp1', 'Temp2'],
 // ['Temp1', 'Temp2'],
 // [numero1, numero2],
  //[numero1, numero2],
   
 // ]);
  
//let hora1 = parseInt(myObj.h1);
let data = google.visualization.arrayToDataTable(myObj);
['Tempo', 'Temp']
   for (var i = 0; i < myObj.length; i++)
    {
     let row = [myObj.Tempo.Hora, myObj.Tempo.Temp];
      // var jsonTemp = document.getElementById('Temp1').innerText= parseInt(myObj);
      data.push(row);
    }

// Create our data table out of JSON data loaded from server.
//var data = new google.visualization.DataTable(jsonData);
      
var options  = {
  'title' : 'TEMP/DIA',
  'width' : 400,
  'height': 300
};
//instanciando e desenhando o grafico linhas
var divgraf = new google.visualization.LineChart(document.getElementById('divgraf'));
divgraf.draw(data,options);

  }
};
xmlhttp.open('GET', "/jsonTemp",true);
xmlhttp.send();
}


function drawChart() {
    var json = $.ajax({
        url: "GetFaturamentoMes",
        dataType: "json",
        success: function (jsonData) {
            var data = new google.visualization.DataTable();
            data.addColumn('number', 'Mês');
            data.addColumn('number', 'Faturamento Por Mês');

            for (var i = 0; i < jsonData.length; i++) {
                mes = jsonData[i].Mes;
                total = jsonData[i].Total;
                data.addRow([mes, total]);
            }
            var options = {
                chart: {
                    title: 'Gráfico de Faturamento Mensal',
                    subtitle: 'Moeda (R$)'
                },
                width: 600,
                height: 300,
                axes: {
                    x: {
                        10: { side: 'top' }
                    }
                }
            };
            var chart = new google.charts.Line(document.getElementById('line_top_x'));
            chart.draw(data, google.charts.Line.convertOptions(options));
        }
    });
}
        