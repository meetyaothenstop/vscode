/* Verify the linked candidate rather than matching source configuration. */
const fs=require('fs'),assert=require('assert');
const artifactDir=process.argv[2]||'.superpowers/multirate';
const map=fs.readFileSync('MDK-ARM/bidreaction/bidreaction.map','utf8');
function address(name){const escaped=name.replace(/[.*+?^${}()|[\]\\]/g,'\\$&');const match=map.match(new RegExp('^\\s+'+escaped+'\\s+0x([0-9a-f]+)\\b','mi'));assert(match,name);return parseInt(match[1],16);}
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
assert(word(0x08000004)===(address('Reset_Handler')|1),'Reset vector');
assert(word(0x08000000+(16+69)*4)===(address('HRTIM1_TIMB_IRQHandler')|1),'HRTIM vector');
const region=map.match(/Execution Region ER_CCMRAM \(Exec base:\s*0x([0-9a-f]+), Load base:\s*0x([0-9a-f]+), Size:\s*0x([0-9a-f]+)/i);
assert(region,'Missing CCM region');
const [exec,load,size]=region.slice(1).map(x=>parseInt(x,16));
assert(exec===0x10000000 && size>0 && size<=0x8000 && load>=0x08000000 && load+size<=0x08020000);
let copied=false;
for(let a=address('Region$$Table$$Base');a+16<=address('Region$$Table$$Limit');a+=16)
 if(word(a+4)===exec){assert(word(a)===load && word(a+8)===size && (word(a+12)|1)===(address('__scatterload_copy')|1));copied=true;}
assert(copied,'Missing CCM startup copy');
for(let a=load;a<load+size;a++)assert(image.has(a),'Incomplete CCM load image');
for(const [name,bytes] of [['oled_commands',4],['oled_data',129],['oled_dma',96],['power_status',1],['__initial_sp',1]]) {
 const a=address(name);assert(a>=0x20000000 && a+bytes<=0x20018000,name+' outside ordinary SRAM');
}
for(const [name,irq] of [['DMA1_Channel1_IRQHandler',11],['I2C1_EV_IRQHandler',31],['I2C1_ER_IRQHandler',32]])
 assert(word(0x08000000+(16+irq)*4)===(address(name)|1),name+' vector');
for(const name of ['PendSV_Handler','Power_ControlIRQ','Power_TimerIRQ','control_slice','control_modulate','control_write','reverse_main_boost_step','reverse_main_boost_hold_step']){
 const a=address(name);assert(a>=0x10000000&&a<0x10008000,name+' outside CCM');
}
const asm=fs.readFileSync(artifactDir+'/main.disasm','utf8');
assert(/PendSV_Handler[\s\S]{0,400}B\.W\s+Power_ControlIRQ/.test(asm),'PendSV does not dispatch worker');
const worker=asm.match(/\n    Power_ControlIRQ\r?\n([\s\S]*?)(?=\n    [A-Za-z_$][\w$]*\r?\n)/);
assert(worker,'Missing disassembled worker');
for(const name of ['reverse_main_boost_step','reverse_main_boost_hold_step'])
 assert(new RegExp('\\bBL(?:\\.W)?\\s+'+name+'\\b').test(worker[1]),'Worker does not call '+name);
assert(address('Power_EmergencyOff')>=0x08000000&&address('Power_EmergencyOff')<0x08020000);
console.log('Actual HEX vectors/CCM copy/both Boost worker branches/2KiB stack/DMA SRAM and AXF byte export: PASS');
