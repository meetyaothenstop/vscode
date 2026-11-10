/* Cold capture/foreground publication must stay out of healthy IRQ bodies. */
const fs=require('fs'),assert=require('assert');
const asm=fs.readFileSync(process.argv[2]||'.superpowers/eventdiag/main.disasm','utf8');
const map=fs.readFileSync('MDK-ARM/bidreaction/bidreaction.map','utf8');
const labels=[...asm.matchAll(/^    ([A-Za-z_$][\w$]*)\r?$/gm)];
function body(name){let i=labels.findIndex(m=>m[1]===name);assert(i>=0,'Missing '+name);return asm.slice(labels[i].index,labels[i+1]?.index||asm.length);}
function address(name){let m=map.match(new RegExp('^\\s+'+name+'\\s+0x([0-9a-f]+)\\b','mi'));assert(m,'Missing linked '+name);return parseInt(m[1],16);}
for(const name of ['event_capture','event_publish','PowerEvent_UIMark']){
 body(name);let a=address(name);assert(a>=0x08000000&&a<0x08020000,name+' should execute from Flash');
}
assert(address('power_event')>=0x20000000&&address('power_event')<0x20018000);
for(const name of ['Power_TimerIRQ','Power_ControlIRQ','raw_checks','control_slice','control_modulate',...(labels.some(m=>m[1]==='control_write')?['control_write']:[])])
 assert(!/\b(?:event_capture|event_publish|event_foreground|PowerEvent_UIMark)\b/.test(body(name)),name+' contains event probes');
assert(/\bevent_publish\b/.test(body('Power_Service')),'Foreground entry is not recorded');
console.log('Actual linked event capture/publication in Flash, data SRAM, no healthy IRQ/worker probes: PASS');
