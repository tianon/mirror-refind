var ANNO={};(function(){var head=document.head?document.head:document.getElementsByTagName("head")[0];var load=function(src){var script=document.createElement('script');script.src=src;head.appendChild(script);};var load_path="";(function(){var path_parts=[];var i=-1;for(var ch=0;ch<window.location.pathname.length;ch+=1){var sample=window.location.pathname[ch];if(sample==="/"){path_parts.push("");i+=1;}
else{path_parts[i]+=sample;}}
var load_path="";if(["localhost","127.0.0.1"].indexOf(window.location.hostname)!==-1){ANNO.DEBUG=true;}
else if(window.location.hostname==="staging.creativecommons.org"){load_path="http://staging.creativecommons.org";ANNO.DEBUG=true;}
else{load_path="http://creativecommons.org";ANNO.DEBUG=false;}
errata_path=load_path+"/errata/json/";for(var k=1;k<path_parts.length-1;k+=1){errata_path+=path_parts[k];if(k!==path_parts.length-2){errata_path+="_";}}
errata_path+=".json";ANNO.errata_json=function(){return errata_path};if(!!ANNO.DEBUG){console.warn("Assuming debug mode.")
console.info("load_path: '"+ load_path+"'");console.info("errata_json: '"+ errata_path+"'");}})();if(ANNO.DEBUG){load(load_path+"/errata/jquery.js");load(load_path+"/errata/errata_engine.js");}})();