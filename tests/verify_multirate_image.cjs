/* Verify the linked candidate rather than matching source configuration. */
const fs=require('fs'),assert=require('assert');
const artifactDir=process.argv[2]||'.superpowers/multirate';
const map=fs.readFileSync('MDK-ARM/bidreaction/bidreaction.map','utf8');
function address(name){const match=map.match(new RegExp('^\\s+'+name+'\\s+0x([0-9a-f]+)\\b','mi'));assert(match,name);return parseInt(match[1],16);}
function hex(path){
 const out=new Map();let upper=0;
 for(const line of fs.readFileSync(path,'utf8').trim().split(/\r?\n/)){
  assert(line[0]===':');const b=Buffer.from(line.slice(1),'hex');
  assert((b.reduce((a,x)=>a+x,0)&255)===0);assert(b.length===b[0]+5);
  if(b[3]===4)upper=b.readUInt16BE(4)*65536;
  else if(b[3]===0){let base=upper+b.readUInt16BE(1);for(let n=0;n<b[0];n++)out.set(base+n,b[4+n]);}
  else assert(b[3]===1||b[3]===5);
 }
 return out;
}
const image=hex('MDK-ARM/bidreaction/bidreaction.hex');
const exported=hex(artifactDir+'/export.hex');
assert(image.size===exported.size);
for(const [a,b] of image)assert(exported.get(a)===b,'AXF export mismatch');
function word(a){let out=0;for(let n=0;n<4;n++){assert(image.has(a+n));out+=image.get(a+n)*2**(8*n);}return out>>>0;}
assert(word(0x08000038)===(address('PendSV_Handler')|1));
assert(word(0x08000000)-address('Stack_Mem')>=2048,'MAIN nested FP IRQ stack below2KiB');
for(const name of ['PendSV_Handler','Power_ControlIRQ','Power_TimerIRQ','control_slice','control_modulate','control_write']){
 const a=address(name);assert(a>=0x10000000&&a<0x10008000,name+' outside CCM');
}
const asm=fs.readFileSync(artifactDir+'/main.disasm','utf8');
assert(/PendSV_Handler[\s\S]{0,400}B\.W\s+Power_ControlIRQ/.test(asm),'PendSV does not dispatch worker');
assert(address('Power_EmergencyOff')>=0x08000000&&address('Power_EmergencyOff')<0x08020000);
console.log('Actual HEX PendSV vector/CCM/worker branch and AXF byte export: PASS');
