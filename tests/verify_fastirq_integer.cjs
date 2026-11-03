const fs=require('fs'),assert=require('assert');
const asm=fs.readFileSync(process.argv[2]||'.superpowers/fastirq-raw/main.disasm','utf8');
const labels=[...asm.matchAll(/^    ([A-Za-z_$][\w$]*)\r?$/gm)];
const names=['Power_TimerIRQ','control_slice','control_modulate','control_write','raw_checks','raw_can_resume','enable_outputs','PWM_EntryIsTimely','PowerProtection_FaultPending'];
// armclang may inline bootstrap into Power_TimerIRQ, whose whole body is checked.
for(const m of labels)if(m[1]==='begin_bootstrap'||m[1].startsWith('$Ven$'))names.push(m[1]);
for(const name of names) {
 const i=labels.findIndex(m=>m[1]===name);assert(i>=0,'Missing '+name);
 const body=asm.slice(labels[i].index,labels[i+1]?.index||asm.length);
 const fp=body.match(/\bV[A-Z]+(?:\.[A-Z0-9]+)?\b/g);
 assert(!fp,name+' uses floating-point instructions: '+fp);
}
console.log('Linked fast IRQ/checks/commit/modulation contain no FP instructions: PASS');
