const fs=require('fs');
const b=fs.readFileSync(process.argv[2]);
const pe=b.readUInt32LE(0x3c), nsec=b.readUInt16LE(pe+6), optsz=b.readUInt16LE(pe+20);
const opt=pe+24, magic=b.readUInt16LE(opt);
const dd=opt+(magic===0x20b?112:96);
const impRva=b.readUInt32LE(dd+8), impSz=b.readUInt32LE(dd+12);
const secs=[];
for(let i=0;i<nsec;i++){const s=opt+optsz+i*40;
 secs.push({va:b.readUInt32LE(s+12),vs:b.readUInt32LE(s+8),raw:b.readUInt32LE(s+20)});}
function r2o(r){for(const s of secs) if(r>=s.va&&r<s.va+Math.max(s.vs,0x1000)) return r-s.va+s.raw; return -1;}
let o=r2o(impRva); const out=[];
while(o>0){const nameRva=b.readUInt32LE(o+12); if(!nameRva)break;
 let n=r2o(nameRva),s='';while(b[n])s+=String.fromCharCode(b[n++]);out.push(s);o+=20;}
console.log(out.join('\n'));
