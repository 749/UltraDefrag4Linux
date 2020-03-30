var p7AB=false;var p7ABi=false;function P7_setAB(){if(!document.getElementById){return;}
var h,tA=navigator.userAgent.toLowerCase();if(window.opera){if(tA.indexOf("opera 5")>-1||tA.indexOf("opera 6")>-1){return;}}
h=String.fromCharCode(60,115,116,121,108,101,32,116,121,112,101,61,34,116,101,120,116,47,99,115,115,34,62,46,112,55,65,66,99,111,110,116,101,110,116,123,100,105,115,112,108,97,121,58,110,111,110,101,59,125,60,47,115,116,121,108,101,62);h+='\n'+String.fromCharCode(60,33,45,45,91,105,102,32,108,116,101,32,73,69,32,55,93,62,60,115,116,121,108,101,62,46,112,55,65,66,44,46,112,55,65,66,32,100,105,118,123,122,111,111,109,58,49,48,48,37,59,125,60,47,115,116,121,108,101,62,60,33,91,101,110,100,105,102,93,45,45,62);h+='\n'+String.fromCharCode(60,33,45,45,91,105,102,32,108,116,101,32,73,69,32,54,93,62,60,115,116,121,108,101,62,46,112,55,65,66,44,46,112,55,65,66,99,111,110,116,101,110,116,44,46,112,55,65,66,116,114,105,103,32,97,123,104,101,105,103,104,116,58,49,37,59,125,60,47,115,116,121,108,101,62,60,33,91,101,110,100,105,102,93,45,45,62);document.write(h);}
P7_setAB();function P7_opAB(){var x,c,tC,ab,tB;if(document.getElementById){ab='p7ABW'+arguments[0];tB=document.getElementById(ab);tB.p7Aba=arguments;x=arguments[3];if(x>0&&x<11){c='p7ABc'+arguments[0]+'_'+x;tC=document.getElementById(c);if(tC){tC.style.display='block';}}
if(!p7ABi){p7ABi=true;if(window.addEventListener){window.addEventListener("load",P7_initAB,false);}
else if(document.addEventListener){document.addEventListener("load",P7_initAB,false);}
else if(window.attachEvent){window.attachEvent("onload",P7_initAB);}
else if(typeof window.onload=='function'){var p7loadit=onload;window.onload=function(){p7loadit();P7_initAB();};}
else{window.onload=P7_initAB;}}}}
function P7_initAB(){var i,j,ab,tB,tD,tA,op,ob,tg;if(!document.getElementById){return;}
for(i=10;i>0;i--){ab='p7ABW'+i;tB=document.getElementById(ab);if(tB){tA=tB.getElementsByTagName("A");tg='p7ABt'+i;for(j=0;j<tA.length;j++){if(tA[j].id&&tA[j].id.indexOf(tg)==0){tA[j].onclick=function(){return P7_ABtrig(this);};tA[j].p7ABstate=0;tA[j].p7ABpr=ab;}}
ob=i+'_'+tB.p7Aba[3];P7_ABopen(ob);}}
p7AB=true;P7_ABurl();P7_ABauto();}
function P7_ABopen(s){var a,g,d='p7ABt'+s;a=document.getElementById(d);g=s.split("_");if(g&&g.length>1&&g[1]==99){a=P7_randAB(g[0]);}
if(g&&g.length>1&&g[1]=='a'){P7_ABall(s);}
else{if(a&&a.p7ABpr){if(a.p7ABstate==0){P7_ABtrig(a);}}}}
function P7_ABclose(s){var a,d='p7ABt'+s;a=document.getElementById(d);if(a&&a.p7ABpr){if(a.p7ABstate==1){P7_ABtrig(a);}}}
function P7_ABclick(s){var a,d='p7ABt'+s;a=document.getElementById(d);if(a&&a.p7ABpr){P7_ABtrig(a);}}
function P7_randAB(r){var i,k,j=0,d,dd,tA,a,rD=new Array();dd='p7ABW'+r;d=document.getElementById(dd);if(d){tA=d.getElementsByTagName("A");for(i=0;i<tA.length;i++){if(tA[i].p7ABpr&&tA[i].p7ABpr==dd){rD[j]=tA[i].id;j++;}}
if(j>0){k=Math.floor(Math.random()*j);a=document.getElementById(rD[k]);}}
return a;}
function P7_ABall(s){var i,m,d,dd,st,et,tA,g=s.split("_");if(g&&g.length==2){st=parseInt(g[0]);if(st){et=st+1;}
else{st=1;et=11;}
m=p7AB;p7AB=false;for(i=st;i<et;i++){dd='p7ABW'+i;d=document.getElementById(dd);if(d){tA=d.getElementsByTagName("A");for(j=0;j<tA.length;j++){if(tA[j].p7ABpr&&tA[j].p7ABpr==dd){if(g[1]=='a'&&tA[j].p7ABstate==0){P7_ABtrig(tA[j]);}
else if(g[1]=='c'&&tA[j].p7ABstate==1){P7_ABtrig(tA[j]);}}}}}
p7AB=m;}}
function P7_ABurl(){var i,h,s,x,d='pab';if(document.getElementById){h=document.location.search;if(h){h=h.replace('?','');s=h.split(/[=&]/g);if(s&&s.length){for(i=0;i<s.length;i+=2){if(s[i]==d){x=s[i+1];if(x){P7_ABopen(x);}}}}}
h=document.location.hash;if(h){x=h.substring(1,h.length);if(x&&x.indexOf("pab")==0){P7_ABopen(x.substring(3));}}}}
function P7_ABtrig(a){var i,j=null,op,pD,ad,aT,m=true,cp='p7ABc'+a.p7ABpr.substring(a.p7ABpr.length-1);var iD=a.id.replace('t','c'),tD=document.getElementById(iD);if(tD){m=false;pD=document.getElementById(a.p7ABpr);op=pD.p7Aba;if(op[4]==1&&op[1]==1&&a.p7ABstate==1){return m;}
tD=pD.getElementsByTagName("DIV");for(i=0;i<tD.length;i++){if(tD[i].id&&tD[i].id.indexOf(cp)>-1){if(tD[i].id==iD){j=i;if(a.className=="p7ABtrig_down"){a.p7ABstate=0;a.className='';P7_ABhide(tD[j],op);}
else{a.p7ABstate=1;a.className="p7ABtrig_down";P7_ABshow(tD[j],op);}}
else{if(op[1]==1){ad=tD[i].id.replace('c','t');aT=document.getElementById(ad);aT.className='';aT.p7ABstate=0;P7_ABhide(tD[i],op);}}}}}
P7_checkEQH();return m;}
function P7_checkEQH(){if(typeof(P7_colH2)=='function'){P7_colH2();}
if(typeof(P7_colH)=='function'){P7_colH();}}
function P7_ABshow(d,op){var h,wd,wP,isIE5=(navigator.appVersion.indexOf("MSIE 5")>-1);if(p7AB&&op[2]==3){d.style.display='block';P7_ABfadeIn(d.id,0);}
else if((p7AB&&op[2]==1||p7AB&&op[2]==2)&&!isIE5){wd=d.id.replace("c","w");wP=document.getElementById(wd);if(P7_hasOverflow(d)||P7_hasOverflow(wP)){d.style.display='block';return;}
wP.style.overflow="hidden";wP.style.height="1px";d.style.display='block';h=d.offsetHeight;P7_ABglide(wd,1,h,op[2]);}
else{d.style.display='block';}}
function P7_ABhide(d,op){var h,wd,wP,isIE5=(navigator.appVersion.indexOf("MSIE 5")>-1);if((p7AB&&op[2]==1||p7AB&&op[2]==2)&&!isIE5){wd=d.id.replace("c","w");wP=document.getElementById(wd);if(d.style.display!="none"){if(P7_hasOverflow(d)||P7_hasOverflow(wP)){d.style.display='none';return;}
h=wP.offsetHeight;wP.style.overflow="hidden";P7_ABglide(wd,h,0,op[2]);}}
else{d.style.display='none';}}
function P7_hasOverflow(ob){var s,m;s=ob.style.overflow;if(!s){if(ob.currentStyle){s=ob.currentStyle.overflow;}
else if(document.defaultView.getComputedStyle(ob,"")){s=document.defaultView.getComputedStyle(ob,"").getPropertyValue("overflow");}}
m=(s&&s=='auto')?true:false;return m;}
function P7_ABfadeIn(id,op){var d=document.getElementById(id);op+=.05;op=(op>=1)?1:op;if((navigator.appVersion.indexOf("MSIE")>-1)){d.style.filter='alpha(opacity='+op*100+')';}
else{d.style.opacity=op;}
if(op<1){setTimeout("P7_ABfadeIn('"+id+"',"+op+")",40);}}
function P7_ABglide(dd,ch,th,p){var w,m,d,wd,wC,tt,dy=10,inc=10,pc=.15;d=document.getElementById(dd);m=(ch<=th)?0:1;if(p==1){tt=Math.abs(parseInt(Math.abs(th)-Math.abs(ch)));inc=(tt*pc<1)?1:tt*pc;}
inc=(m==1)?inc*-1:inc;d.style.height=ch+"px";if(ch==th){if(th==0){wd=d.id.replace("w","c");wC=document.getElementById(wd);wC.style.display="none";d.style.height="auto";}
else{d.style.height="auto";}
P7_checkEQH();}
else{ch+=inc;if(m==0){ch=(ch>=th)?th:ch;}
else{ch=(ch<=th)?th:ch;}
if(d.p7abG){clearTimeout(d.p7abG);}
d.p7abG=setTimeout("P7_ABglide('"+dd+"',"+ch+","+th+","+p+")",dy);}}
function P7_ABauto(){var i,k,ab,tB,wH,tr,pp;wH=window.location.href;for(k=1;k<11;k++){tr=null;ab='p7ABW'+k;tB=document.getElementById(ab);if(tB&&tB.p7Aba[5]&&tB.p7Aba[5]==1){tA=tB.getElementsByTagName('A');if(tA){for(i=0;i<tA.length;i++){if(tA[i].href==wH){if(tA[i].p7ABpr){tr=tA[i].id.replace('p7ABt','');break;}
else{tA[i].className="p7ap_currentmark";pp=tA[i].parentNode;while(pp){if(pp.id&&pp.id.indexOf('p7ABc')==0){tr=pp.id.replace('p7ABc','');break;}
pp=pp.parentNode;}
break;}}}
if(tr){P7_ABopen(tr);}
else{if(typeof(p7ABcm)!='undefined'&&p7ABcm.length){for(i=0;i<p7ABcm.length;i++){if(p7ABcm[i]&&p7ABcm[i].length>0){if(k==p7ABcm[i].charAt(0)){P7_ABopen(p7ABcm[i]);}}}}}}}}}