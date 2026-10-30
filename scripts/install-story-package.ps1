[CmdletBinding()]
param(
    [string]$MapsDirectory='D:\SteamLibrary\steamapps\common\Cry of Fear\cryoffear\maps',
    [switch]$AllDoorCandidates,
    [switch]$SkipMenu,
    [switch]$VerifyOnly,
    [switch]$Remove
)
$ErrorActionPreference='Stop'
function Get-PatchSha([byte[]]$Bytes) {
    $digest=[Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($digest.ComputeHash($Bytes))).Replace('-','').ToLowerInvariant() }
    finally { $digest.Dispose() }
}
function Get-PatchFileSha([string]$Path) { return Get-PatchSha ([IO.File]::ReadAllBytes($Path)) }
function New-PatchedBsp([byte[]]$Bytes,$Plan) {
    # Same lossless append algorithm as install-shared-note.ps1; source/output
    # pins and CreateNew guards are retained. No Get-FileHash module dependency.
    if ((Get-PatchSha $Bytes) -cne $Plan.sourceSha256) { throw 'Original BSP SHA256 mismatch. Nothing installed.' }
    if ($Bytes.Length -lt 124 -or [BitConverter]::ToInt32($Bytes,0) -ne 30) { throw 'Expected complete BSP30.' }
    $oldOffset=[BitConverter]::ToInt32($Bytes,4); $oldLength=[BitConverter]::ToInt32($Bytes,8)
    if ($oldOffset -lt 124 -or $oldLength -lt 1 -or ([long]$oldOffset+$oldLength) -gt $Bytes.Length) { throw 'Invalid entity lump.' }
    $textLength=0
    while ($textLength -lt $oldLength -and $Bytes[$oldOffset+$textLength] -ne 0) { $textLength++ }
    if ($textLength -eq $oldLength) { throw 'Missing entity terminator.' }
    $encoding=[Text.Encoding]::GetEncoding(28591)
    $text=$encoding.GetString($Bytes,$oldOffset,$textLength)
    $previousStart=$text.Length
    foreach ($edit in @($Plan.edits | Sort-Object -Property start -Descending)) {
        if ($edit.start -lt 0 -or $edit.end -le $edit.start -or $edit.end -gt $previousStart -or $text[$edit.start] -cne '{' -or $text[$edit.end-1] -cne '}') { throw 'Invalid/overlapping entity edit. Nothing installed.' }
        $text=$text.Remove($edit.start,$edit.end-$edit.start).Insert($edit.start,$edit.to)
        $previousStart=$edit.start
    }
    $entityBytes=$encoding.GetBytes($text+$Plan.appendText+[char]0)
    $newOffset=[int]([Math]::Ceiling($Bytes.Length/4.0)*4)
    $totalLength=$newOffset+$entityBytes.Length
    $models=@($Plan.modelClones | Where-Object { $null -ne $_ })
    if ($models.Count) {
        $oldModelOffset=[BitConverter]::ToInt32($Bytes,116); $oldModelLength=[BitConverter]::ToInt32($Bytes,120)
        if ($oldModelOffset -lt 124 -or $oldModelLength -lt 64 -or $oldModelLength % 64 -ne 0 -or ([long]$oldModelOffset+$oldModelLength) -gt $Bytes.Length) { throw 'Invalid original model lump.' }
        $newModelOffset=[int]([Math]::Ceiling($totalLength/4.0)*4)
        $newModelLength=$oldModelLength+$models.Count*64
        $totalLength=$newModelOffset+$newModelLength
    }
    $output=New-Object byte[] $totalLength
    [Array]::Copy($Bytes,0,$output,0,$Bytes.Length)
    [Array]::Copy($entityBytes,0,$output,$newOffset,$entityBytes.Length)
    [Array]::Copy([BitConverter]::GetBytes($newOffset),0,$output,4,4)
    [Array]::Copy([BitConverter]::GetBytes([int]$entityBytes.Length),0,$output,8,4)
    if ($models.Count) {
        [Array]::Copy($Bytes,$oldModelOffset,$output,$newModelOffset,$oldModelLength)
        for ($i=0; $i -lt $models.Count; $i++) {
            $model=$models[$i]; $at=$newModelOffset+$oldModelLength+$i*64
            if ($model.sourceModel -lt 1 -or ($model.sourceModel+1)*64 -gt $oldModelLength -or $model.index -ne ($oldModelLength/64+$i) -or $model.mins.Count -ne 3 -or $model.maxs.Count -ne 3) { throw 'Invalid readiness model clone.' }
            [Array]::Copy($Bytes,($oldModelOffset+$model.sourceModel*64),$output,$at,64)
            for ($k=0; $k -lt 3; $k++) {
                $lo=[single]$model.mins[$k]; $hi=[single]$model.maxs[$k]
                if ([single]::IsNaN($lo) -or [single]::IsNaN($hi) -or [single]::IsInfinity($lo) -or [single]::IsInfinity($hi) -or $lo -ge $hi) { throw 'Invalid readiness bounds.' }
                [Array]::Copy([BitConverter]::GetBytes($lo),0,$output,($at+$k*4),4)
                [Array]::Copy([BitConverter]::GetBytes($hi),0,$output,($at+12+$k*4),4)
                [Array]::Copy([BitConverter]::GetBytes([single]0),0,$output,($at+24+$k*4),4)
            }
        }
        [Array]::Copy([BitConverter]::GetBytes([int]$newModelOffset),0,$output,116,4)
        [Array]::Copy([BitConverter]::GetBytes([int]$newModelLength),0,$output,120,4)
    }
    if ((Get-PatchSha $output) -cne $Plan.outputSha256 -or $output.Length -ne $Plan.fileBytes) { throw 'Generated BSP SHA256/size mismatch. Nothing installed.' }
    return ,$output
}
# coop-fixes-008: Simon's player model is built here from this PC's own
# police1.mdl and cutscene\simon_clothes.mdl (see scripts\simon-model.mjs).
# The catalog carries the bone map and the exact per-bone transforms; this
# code only does multiply-adds in doubles rounded to float32, so the output is
# byte-identical everywhere and is checked against the catalog SHA256.
function New-SimonModel([byte[]]$Police,[byte[]]$Simon,[int[]]$BoneMap,[double[]]$Transforms) {
    if (-not ('StoryCoopSimonModel' -as [type])) {
        Add-Type -Language CSharp -TypeDefinition @'
using System;
public static class StoryCoopSimonModel {
    static int R(byte[] b,int o){return BitConverter.ToInt32(b,o);}
    static void W(byte[] b,int o,int v){Buffer.BlockCopy(BitConverter.GetBytes(v),0,b,o,4);}
    static void Point(byte[] o,int at,double[] xf,int m,bool translate){
        double v0=BitConverter.ToSingle(o,at),v1=BitConverter.ToSingle(o,at+4),v2=BitConverter.ToSingle(o,at+8);
        float[] r=new float[3];
        for(int a=0;a<3;++a){double t=(double)(xf[m+a*3]*v0);t=(double)(t+(double)(xf[m+a*3+1]*v1));t=(double)(t+(double)(xf[m+a*3+2]*v2));if(translate)t=(double)(t+xf[m+9+a]);r[a]=(float)t;}
        for(int a=0;a<3;++a)Buffer.BlockCopy(BitConverter.GetBytes(r[a]),0,o,at+a*4,4);
    }
    public static byte[] Build(byte[] police,byte[] simon,int[] map,double[] xf){
        if(xf.Length!=map.Length*12)throw new ArgumentException("transforms");
        int X=(police.Length+15)&~15;byte[] o=new byte[X+simon.Length];Buffer.BlockCopy(police,0,o,0,police.Length);Buffer.BlockCopy(simon,0,o,X,simon.Length);
        int nt=R(simon,180),ti=R(simon,184),td=R(simon,188),nsr=R(simon,192),nsf=R(simon,196),si=R(simon,200),nbp=R(simon,204),bpi=R(simon,208);
        for(int i=0;i<nt;++i){int t=X+ti+80*i;W(o,t+76,R(o,t+76)+X);}
        for(int i=0;i<nbp;++i){
            int bp=X+bpi+76*i,nm=R(o,bp+64);W(o,bp+72,R(o,bp+72)+X);int mi=R(o,bp+72);
            for(int j=0;j<nm;++j){
                int mo=mi+112*j,nmesh=R(o,mo+72),meshi=R(o,mo+76),nv=R(o,mo+80),vinfo=R(o,mo+84),vi=R(o,mo+88),nn=R(o,mo+92),ninfo=R(o,mo+96),ni=R(o,mo+100);
                foreach(int k in new int[]{76,84,88,96,100,108}){int v=R(o,mo+k);if(v!=0)W(o,mo+k,v+X);}
                for(int m=0;m<nmesh;++m){int me=X+meshi+20*m;W(o,me+4,R(o,me+4)+X);if(R(o,me+16)!=0)W(o,me+16,R(o,me+16)+X);}
                for(int k=0;k<nv;++k){int b=o[X+vinfo+k];if(b>=map.Length)throw new ArgumentException("vertex bone");Point(o,X+vi+12*k,xf,b*12,true);o[X+vinfo+k]=(byte)map[b];}
                for(int k=0;k<nn;++k){int b=o[X+ninfo+k];if(b>=map.Length)throw new ArgumentException("normal bone");Point(o,X+ni+12*k,xf,b*12,false);o[X+ninfo+k]=(byte)map[b];}
            }
        }
        W(o,180,nt);W(o,184,X+ti);W(o,188,X+td);W(o,192,nsr);W(o,196,nsf);W(o,200,X+si);W(o,204,nbp);W(o,208,X+bpi);
        for(int i=8;i<72;++i)o[i]=0;byte[] name=System.Text.Encoding.ASCII.GetBytes("simon.mdl");Buffer.BlockCopy(name,0,o,8,name.Length);W(o,72,o.Length);
        // v0.7: names the client's outfit swap does not look for (see scripts\simon-model.mjs).
        for(int i=0;i<nt;++i){int t=X+ti+80*i;string n=System.Text.Encoding.ASCII.GetString(o,t,64);int z=n.IndexOf('\0');if(z>=0)n=n.Substring(0,z);string to=n=="hoodie_grey.bmp"?"hoodie_coop.bmp":n=="jeans.bmp"?"jeanss.bmp":n=="head_face.bmp"?"head_coop.bmp":null;if(to!=null){for(int k=0;k<64;++k)o[t+k]=0;byte[] nb=System.Text.Encoding.ASCII.GetBytes(to);Buffer.BlockCopy(nb,0,o,t,nb.Length);}}
        return o;
    }
}
'@
    }
    return ,[StoryCoopSimonModel]::Build($Police,$Simon,$BoneMap,$Transforms)
}
# v0.7: one player model per outfit from Simon's model and the game's own costume
# textures (median cut in integers, the same steps as scripts\simon-model.mjs).
function New-SimonOutfitModel([byte[]]$Simon,[string]$Name,[string[]]$Parts,[object[]]$Textures) {
    if (-not ('StoryCoopOutfitModel' -as [type])) {
        Add-Type -TypeDefinition @'
using System;using System.Collections.Generic;
public static class StoryCoopOutfitModel {
    static int R(byte[] b,int o){return BitConverter.ToInt32(b,o);}
    static void Quantize(byte[] t,int w,int h,byte[] o,int at){
        if(t[2]!=2||t[16]!=24||BitConverter.ToUInt16(t,12)!=w||BitConverter.ToUInt16(t,14)!=h)throw new ArgumentException("texture");
        int start=18+t[0];bool top=(t[17]&0x20)!=0;int[] count=new int[32768],sr=new int[32768],sg=new int[32768],sb=new int[32768];int[] code=new int[w*h];
        for(int y=0;y<h;++y){int row=start+(top?y:h-1-y)*w*3;for(int x=0;x<w;++x){int b=t[row+x*3],g=t[row+x*3+1],r=t[row+x*3+2],c=((r>>3)<<10)|((g>>3)<<5)|(b>>3);code[y*w+x]=c;++count[c];sr[c]+=r;sg[c]+=g;sb[c]+=b;}}
        List<int> first=new List<int>();for(int c=0;c<32768;++c)if(count[c]!=0)first.Add(c);
        List<int[]> boxes=new List<int[]>();boxes.Add(first.ToArray());
        while(boxes.Count<256){
            int best=-1,bestAxis=0;long bestScore=-1;
            for(int i=0;i<boxes.Count;++i){int[] box=boxes[i];if(box.Length<2)continue;int lr=31,lg=31,lb=31,hr=0,hg=0,hb=0;long pop=0;
                foreach(int c in box){pop+=count[c];int r=(c>>10)&31,g=(c>>5)&31,b=c&31;if(r<lr)lr=r;if(r>hr)hr=r;if(g<lg)lg=g;if(g>hg)hg=g;if(b<lb)lb=b;if(b>hb)hb=b;}
                int rr=hr-lr,rg=hg-lg,rb=hb-lb,max=Math.Max(rr,Math.Max(rg,rb));long score=pop*max;if(score>bestScore){bestScore=score;best=i;bestAxis=rr==max?0:rg==max?1:2;}}
            if(best<0)break;
            int[] sorted=boxes[best];int[] keys=new int[sorted.Length];
            for(int i=0;i<keys.Length;++i){int c=sorted[i];int v=bestAxis==0?(c>>10)&31:bestAxis==1?(c>>5)&31:c&31;keys[i]=v*32768+c;}
            Array.Sort(keys);
            long all=0;foreach(int key in keys)all+=count[key%32768];long cum=0;int k=0;for(;k<keys.Length;++k){cum+=count[keys[k]%32768];if(2*cum>=all)break;}if(k>=keys.Length-1)k=keys.Length-2;
            int[] left=new int[k+1],right=new int[keys.Length-k-1];for(int i=0;i<keys.Length;++i){if(i<=k)left[i]=keys[i]%32768;else right[i-k-1]=keys[i]%32768;}
            boxes[best]=left;boxes.Add(right);
        }
        byte[] lut=new byte[32768];int pal=at+w*h;for(int i=0;i<768;++i)o[pal+i]=0;
        for(int i=0;i<boxes.Count;++i){long n=0,r=0,g=0,b=0;foreach(int c in boxes[i]){n+=count[c];r+=sr[c];g+=sg[c];b+=sb[c];lut[c]=(byte)i;}o[pal+i*3]=(byte)((r+n/2)/n);o[pal+i*3+1]=(byte)((g+n/2)/n);o[pal+i*3+2]=(byte)((b+n/2)/n);}
        for(int i=0;i<w*h;++i)o[at+i]=lut[code[i]];
    }
    public static byte[] Build(byte[] simon,string name,string[] parts,object[] textures){
        byte[] o=(byte[])simon.Clone();int nt=R(o,180),ti=R(o,184);
        for(int p=0;p<parts.Length;++p){string want=parts[p]=="hoodie"?"hoodie_coop.bmp":parts[p]=="jeans"?"jeanss.bmp":parts[p]=="face"?"head_coop.bmp":null;if(want==null)throw new ArgumentException("part");bool done=false;
            for(int i=0;i<nt;++i){int t=ti+80*i;string n=System.Text.Encoding.ASCII.GetString(o,t,64);int z=n.IndexOf('\0');if(z>=0)n=n.Substring(0,z);if(n!=want)continue;Quantize((byte[])textures[p],R(o,t+68),R(o,t+72),o,R(o,t+76));done=true;}
            if(!done)throw new ArgumentException("target");}
        for(int i=8;i<72;++i)o[i]=0;byte[] nb=System.Text.Encoding.ASCII.GetBytes(name+".mdl");Buffer.BlockCopy(nb,0,o,8,nb.Length);
        return o;
    }
}
'@
    }
    return ,[StoryCoopOutfitModel]::Build($Simon,$Name,$Parts,$Textures)
}
# coop-fixes-010: the CO-OP menu title is shipped only as the XOR of the
# game's own title and the new one (raw deflate); it is rebuilt from this PC's
# original, which is checked by SHA256 first.
function Expand-XorDelta([byte[]]$Original,[byte[]]$Delta) {
    $packed=New-Object IO.MemoryStream(,$Delta); $inflate=New-Object IO.Compression.DeflateStream($packed,[IO.Compression.CompressionMode]::Decompress); $output=New-Object IO.MemoryStream
    try { $inflate.CopyTo($output) } finally { $inflate.Dispose() }
    $bytes=$output.ToArray(); if ($bytes.Length -ne $Original.Length) { throw 'Delta length mismatch.' }
    for ($i=0; $i -lt $bytes.Length; $i++) { $bytes[$i]=$bytes[$i] -bxor $Original[$i] }
    return ,$bytes
}
function Write-NewPatchFile([string]$Path,[byte[]]$Bytes) {
    $temporary=$Path+'.storycoop-'+[Guid]::NewGuid().ToString('N')+'.tmp'
    $stream=[IO.File]::Open($temporary,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try {
        try { $stream.Write($Bytes,0,$Bytes.Length) } finally { $stream.Dispose() }
        [IO.File]::Move($temporary,$Path)
    } finally { if ([IO.File]::Exists($temporary)) { [IO.File]::Delete($temporary) } }
}
$maps=(Resolve-Path -LiteralPath $MapsDirectory).ProviderPath
if (-not [IO.Directory]::Exists($maps)) { throw 'MapsDirectory must be a directory.' }
$catalog=([IO.File]::ReadAllText((Join-Path $PSScriptRoot 'release.json')) | ConvertFrom-Json)
if ($catalog.schemaVersion -notin @(1,2) -or $catalog.release -cnotin @('story-coop-experimental-rc1','connected-story-campaign-rc2','story-opening-rc3','story-state-rc4')) { throw 'Unexpected package catalog.' }
if (-not $Remove) {
    foreach ($dependency in @($catalog.dependencies | Where-Object { $null -ne $_ })) {
        if ($dependency.map -cnotmatch '^(s2_|cof_campaign_)[a-z0-9_]{1,27}$' -or $dependency.map.Length -gt 31) { throw 'Unsafe dependency name.' }
        $dependencyPath=Join-Path $maps ($dependency.map+'.bsp')
        if (-not [IO.File]::Exists($dependencyPath) -or (Get-PatchFileSha $dependencyPath) -cne $dependency.outputSha256) { throw "Required RC2 map missing or changed: $dependencyPath. Install the original RC2 package first. Nothing installed." }
    }
}
$selected=@($catalog.maps | Where-Object { $catalog.schemaVersion -eq 2 -or $Remove -or $AllDoorCandidates -or $_.defaultInstall })
Write-Output "Preflighting $($selected.Count) map files. Original maps and other release aliases are retained."
$prepared=New-Object 'System.Collections.Generic.List[object]'
foreach ($entry in $selected) {
    if ($entry.map -cnotmatch '^(c_|s2_|s3_|s4_|cof_campaign_)[a-z0-9_]{1,30}$' -or $entry.map.Length -gt 31 -or $entry.sourceMap -cnotmatch '^c_[a-z0-9_]{1,30}$' -or $entry.map -ceq $entry.sourceMap) { throw 'Unsafe map name.' }
    $destination=Join-Path $maps ($entry.map+'.bsp')
    $exists=[IO.File]::Exists($destination)
    if ($exists -and (Get-PatchFileSha $destination) -cne $entry.outputSha256) { throw "Unknown/changed file retained: $destination. Nothing installed or removed." }
    if ($Remove) { $prepared.Add(@{path=$destination; exists=$exists}); continue }
    $manifestPath=Join-Path $PSScriptRoot ('plans\'+$entry.map+'.json')
    if ((Get-PatchFileSha $manifestPath) -cne $entry.planSha256) { throw 'Changed package plan. Nothing installed.' }
    $plan=([IO.File]::ReadAllText($manifestPath) | ConvertFrom-Json)
    if ($plan.schemaVersion -ne $catalog.schemaVersion -or $plan.map -cne $entry.map -or $plan.sourceSha256 -cne $entry.sourceSha256 -or $plan.outputSha256 -cne $entry.outputSha256) { throw 'Plan/catalog mismatch.' }
    $bytes=New-PatchedBsp ([IO.File]::ReadAllBytes((Join-Path $maps ($entry.sourceMap+'.bsp')))) $plan
    # Keep only metadata after full preflight; a campaign can exceed 3 GB.
    $prepared.Add(@{path=$destination; exists=$exists; source=(Join-Path $maps ($entry.sourceMap+'.bsp')); plan=$plan; hash=$entry.outputSha256})
    $bytes=$null
}
if ($catalog.release -ceq 'story-state-rc4') {
    $adapter=$catalog.runtimeAdapter
    if ($null -eq $adapter -or $adapter.file -cne 'storycoop_rc4.dll' -or $adapter.sha256 -cnotmatch '^[a-f0-9]{64}$' -or $adapter.fileBytes -lt 1) { throw 'Invalid RC4 runtime adapter catalog.' }
    $adapterDestination=[IO.Path]::GetFullPath((Join-Path $maps '..\cl_dlls\storycoop_rc4.dll'))
    if (-not [IO.Directory]::Exists([IO.Path]::GetDirectoryName($adapterDestination))) { throw 'Existing game cl_dlls directory required.' }
    $adapterExists=[IO.File]::Exists($adapterDestination)
    if ($adapterExists -and (Get-PatchFileSha $adapterDestination) -cne $adapter.sha256) { throw 'Unknown/changed RC4 adapter retained. Nothing installed or removed.' }
    if ($Remove) { $prepared.Add(@{path=$adapterDestination; exists=$adapterExists}) }
    else {
        $adapterBytes=[IO.File]::ReadAllBytes((Join-Path $PSScriptRoot ('native\'+$adapter.file)))
        if ((Get-PatchSha $adapterBytes) -cne $adapter.sha256 -or $adapterBytes.Length -ne $adapter.fileBytes) { throw 'Changed RC4 runtime artifact. Nothing installed.' }
        $prepared.Add(@{path=$adapterDestination; exists=$adapterExists; artifactBytes=$adapterBytes; fileBytes=$adapterBytes.Length})
    }
}
# coop-fixes-005 extras: main-menu Custom Campaign entries (own text files) and
# Simon's player model (005-007 copied models\player.mdl, which is the police
# model; 008 builds the real Simon model from this PC's game files). Same
# guards as maps: unknown/changed files are never overwritten or removed.
$gameRoot=[IO.Path]::GetFullPath((Join-Path $maps '..'))
foreach ($extra in @($catalog.extras | Where-Object { $null -ne $_ })) {
    if ($null -ne $extra.replaces) {
        # coop-fixes-010: the CO-OP title replaces a game file; the original is
        # kept beside it (.storycoop-original) and put back by -Remove.
        # coop-fixes-013: the patched client.dll (Host/Join Simon's Story menu,
        # player list on every co-op map) is a second replaced file, same rules.
        $replaced=@{'gfx/vgui/menu_logo.tga'=@('extras/menu_logo.coop-delta','CO-OP menu title skipped: gfx\vgui\menu_logo.tga is missing or a different version. The game''s own title stays.');'cl_dlls/client.dll'=@('extras/client.coop-delta','Simon''s Story main menu skipped: cl_dlls\client.dll is a different version. Use Custom Campaign in the game''s own menu.')}
        if (-not $replaced.ContainsKey($extra.target) -or $extra.file -cne $replaced[$extra.target][0] -or $extra.sha256 -cnotmatch '^[a-f0-9]{64}$' -or $extra.replaces.sha256 -cnotmatch '^[a-f0-9]{64}$' -or $extra.fileBytes -lt 1 -or $extra.replaces.fileBytes -ne $extra.fileBytes) { throw 'Invalid extra file catalog.' }
        $extraPath=Join-Path $gameRoot ($extra.target.Replace('/','\')); $keep=$extraPath+'.storycoop-original'
        $current=''; if ([IO.File]::Exists($extraPath)) { $current=Get-PatchFileSha $extraPath }
        $keptSha=''; if ([IO.File]::Exists($keep)) { $keptSha=Get-PatchFileSha $keep }
        if ($keptSha -and $keptSha -cne $extra.replaces.sha256) { throw "Unknown/changed file retained: $keep. Nothing installed or removed." }
        if ($current -ceq $extra.sha256 -and -not $keptSha) { throw "Replaced file has no verified original beside it: $extraPath. Nothing installed or removed." }
        if ($Remove) {
            if ($current -ceq $extra.sha256) { $prepared.Add(@{path=$extraPath; restore=$keep; restoreBytes=[IO.File]::ReadAllBytes($keep); extra=$true}) }
            elseif ($keptSha -and $current -cne $extra.replaces.sha256) { throw "Unknown/changed file retained: $extraPath. Nothing installed or removed." }
            # Otherwise it was never replaced by this package: left alone.
            continue
        }
        if ($current -ceq $extra.sha256) { $prepared.Add(@{path=$extraPath; exists=$true; extra=$true}); continue }
        if ($keptSha -and $current -cne $extra.replaces.sha256) { throw "Unknown/changed file retained: $extraPath. Nothing installed or removed." }
        if ($current -cne $extra.replaces.sha256) {
            if ($extra.optional) { Write-Output $replaced[$extra.target][1]; continue }
            throw 'Original file missing or changed. Nothing installed.'
        }
        $originalBytes=[IO.File]::ReadAllBytes($extraPath); $extraBytes=$null
        try { $extraBytes=Expand-XorDelta $originalBytes ([IO.File]::ReadAllBytes((Join-Path $PSScriptRoot ($extra.file.Replace('/','\'))))) } catch { $extraBytes=$null }
        if ($null -eq $extraBytes -or (Get-PatchSha $extraBytes) -cne $extra.sha256 -or $extraBytes.Length -ne $extra.fileBytes) { throw 'Generated extra mismatch. Nothing installed.' }
        $prepared.Add(@{path=$extraPath; exists=$false; replace=$keep; originalBytes=$originalBytes; artifactBytes=$extraBytes; fileBytes=$extraBytes.Length; extra=$true})
        continue
    }
    # v0.5.1: the Create Server entries (maps/Chapter-N-*.bsp, maps/Load-Save-Slot-N.bsp), the
    # join box picture and the Create Server options (sc_setup.scr, rewritten by the game: mutable).
    if ($extra.target -cnotmatch '^(maps/storycoop_[a-z0-9]+\.(custom|coopm)|models/player/simon(_[a-z]+)?/simon(_[a-z]+)?\.mdl|gfx/vgui/menu/sc_(host|join|mdl)(_mo)?\.tga|gfx/vgui/sc_logo\.tga|maps/(Chapter-[0-7](-[A-Za-z]+)+|Load-Save-Slot-[1-5])\.bsp|gfx/vgui/sc_(joinbox|mb[a-z0-9]+)\.tga|sc_setup\.scr)$' -or $extra.sha256 -cnotmatch '^[a-f0-9]{64}$' -or $extra.fileBytes -lt 1) { throw 'Invalid extra file catalog.' }
    $extraPath=Join-Path $gameRoot ($extra.target.Replace('/','\'))
    $extraExists=[IO.File]::Exists($extraPath)
    # v0.7: a kept sc_setup.scr without the 'Unlock extras' option (sc_extras) is replaced once, so the option shows up.
    if ($extra.mutable -and $extra.target -ceq 'sc_setup.scr') { $keepSetup=$extraExists -and ([IO.File]::ReadAllText($extraPath) -match 'sc_extras'); $setupReplace=$extraExists -and -not $keepSetup; if ($Remove -or $keepSetup) { $prepared.Add(@{path=$extraPath; exists=$extraExists; extra=$true}); continue } }
    elseif ($extraExists -and (Get-PatchFileSha $extraPath) -cne $extra.sha256) { throw "Unknown/changed file retained: $extraPath. Nothing installed or removed." }
    if ($Remove) { $prepared.Add(@{path=$extraPath; exists=$extraExists; extra=$true}); continue }
    if ($null -ne $extra.deltaFrom) {
        # v0.1: the CO-OP title is a NEW file (gfx\vgui\sc_logo.tga) rebuilt from
        # this PC's own title: gfx\vgui\menu_logo.tga, or the copy an older build
        # kept beside it. The game's own title is never replaced.
        # v0.5.2: the join box picture is rebuilt the same way from the game's own
        # computer login screen (scripts/menu/make-joinbox.py).
        $deltaKinds=@{'gfx/vgui/sc_logo.tga'=@('extras/sc_logo.coop-delta','gfx/vgui/menu_logo.tga','CO-OP menu title skipped: gfx\vgui\menu_logo.tga is missing or a different version. The game''s own title is shown.');'gfx/vgui/sc_joinbox.tga'=@('extras/sc_joinbox.coop-delta','gfx/vgui/puzzle/computer/computer_logon.tga','Join box picture skipped: gfx\vgui\puzzle\computer\computer_logon.tga is missing or a different version.')}
        # v0.7: the Player Model box pictures, one per outfit, from the same computer screen.
        if ($extra.target -cmatch '^gfx/vgui/(sc_mb[a-z0-9]+)\.tga$') { $deltaKinds[$extra.target]=@(('extras/'+$Matches[1]+'.coop-delta'),'gfx/vgui/puzzle/computer/computer_logon.tga','Player Model box picture skipped: gfx\vgui\puzzle\computer\computer_logon.tga is missing or a different version.') }
        if (-not $deltaKinds.ContainsKey($extra.target) -or $extra.file -cne $deltaKinds[$extra.target][0] -or $extra.deltaFrom.target -cne $deltaKinds[$extra.target][1] -or $extra.deltaFrom.sha256 -cnotmatch '^[a-f0-9]{64}$' -or $extra.deltaFrom.fileBytes -ne $extra.fileBytes) { throw 'Invalid extra file catalog.' }
        if ($extraExists) { $prepared.Add(@{path=$extraPath; exists=$true; extra=$true}); continue }
        $titlePath=Join-Path $gameRoot ($extra.deltaFrom.target.Replace('/','\')); $titleSource=$null
        foreach ($candidate in @($titlePath,($titlePath+'.storycoop-original'))) { if ([IO.File]::Exists($candidate) -and (Get-PatchFileSha $candidate) -ceq $extra.deltaFrom.sha256) { $titleSource=$candidate; break } }
        if (-not $titleSource) { Write-Output $deltaKinds[$extra.target][2]; continue }
        $extraBytes=$null
        try { $extraBytes=Expand-XorDelta ([IO.File]::ReadAllBytes($titleSource)) ([IO.File]::ReadAllBytes((Join-Path $PSScriptRoot ($extra.file.Replace('/','\'))))) } catch { $extraBytes=$null }
        if ($null -eq $extraBytes -or (Get-PatchSha $extraBytes) -cne $extra.sha256 -or $extraBytes.Length -ne $extra.fileBytes) { throw 'Generated extra mismatch. Nothing installed.' }
    } elseif ($extra.file) {
        if ($extra.file -cnotmatch '^extras/(storycoop_[a-z0-9]+\.(custom|coopm)|sc_(host|join|mdl)(_mo)?\.tga|sc_joinbox\.tga|sc_setup\.scr)$') { throw 'Invalid extra package file.' }
        $extraBytes=[IO.File]::ReadAllBytes((Join-Path $PSScriptRoot ($extra.file.Replace('/','\'))))
    } elseif ($extra.build -ceq 'simon-outfit-model') {
        # v0.7: built from the Simon model made just above and this PC's costume textures. Optional: a missing or changed one only leaves that outfit out of the box.
        $outfitName=[IO.Path]::GetFileNameWithoutExtension($extra.target); $outfitParts=@(); $outfitTextures=@(); $outfitOk=$null -ne $simonBuilt
        foreach ($source in @($extra.sources)) {
            if ($source.path -cnotmatch '^models/costumes/[A-Za-z_]+/[A-Za-z_]+\.(tga|TGA)$' -or $source.part -cnotmatch '^(hoodie|jeans|face)$') { throw 'Invalid extra file catalog.' }
            $sourcePath=Join-Path $gameRoot ($source.path.Replace('/','\'))
            if (-not [IO.File]::Exists($sourcePath) -or (Get-PatchFileSha $sourcePath) -cne $source.sha256) { $outfitOk=$false; break }
            $outfitParts+=$source.part; $outfitTextures+=,[IO.File]::ReadAllBytes($sourcePath)
        }
        $extraBytes=$null; if ($outfitOk) { try { $extraBytes=New-SimonOutfitModel $simonBuilt $outfitName ([string[]]$outfitParts) ([object[]]$outfitTextures) } catch { $extraBytes=$null } }
        if ($null -eq $extraBytes -or (Get-PatchSha $extraBytes) -cne $extra.sha256 -or $extraBytes.Length -ne $extra.fileBytes) { Write-Output ('Outfit model skipped ('+$outfitName+'): its costume files are missing or a different version.'); continue }
    } elseif ($extra.build) {
        if ($extra.build -cne 'simon-player-model' -or $extra.sources.police -cne 'models/player/police1/police1.mdl' -or $extra.sources.simon -cne 'models/cutscene/simon_clothes.mdl' -or $extra.sources.policeSha256 -cnotmatch '^[a-f0-9]{64}$' -or $extra.sources.simonSha256 -cnotmatch '^[a-f0-9]{64}$') { throw 'Invalid extra build.' }
        $policePath=Join-Path $gameRoot 'models\player\police1\police1.mdl'; $simonPath=Join-Path $gameRoot 'models\cutscene\simon_clothes.mdl'
        if (-not [IO.File]::Exists($policePath) -or -not [IO.File]::Exists($simonPath) -or (Get-PatchFileSha $policePath) -cne $extra.sources.policeSha256 -or (Get-PatchFileSha $simonPath) -cne $extra.sources.simonSha256) {
            if ($extra.optional) { Write-Output 'Simon player model skipped: police1.mdl or cutscene\simon_clothes.mdl is missing or a different version. Players keep their own models.'; continue }
            throw 'Required extra source missing or changed. Nothing installed.'
        }
        $xfBytes=[Convert]::FromBase64String($extra.transforms); $xf=New-Object 'double[]' ($xfBytes.Length/8); for ($i=0; $i -lt $xf.Length; $i++) { $xf[$i]=[BitConverter]::ToDouble($xfBytes,$i*8) }
        $extraBytes=$null; try { $extraBytes=New-SimonModel ([IO.File]::ReadAllBytes($policePath)) ([IO.File]::ReadAllBytes($simonPath)) ([int[]]@($extra.boneMap)) $xf } catch { $extraBytes=$null }
        if ($null -eq $extraBytes -or (Get-PatchSha $extraBytes) -cne $extra.sha256 -or $extraBytes.Length -ne $extra.fileBytes) {
            if ($extra.optional) { Write-Output 'Simon player model skipped: the model built on this PC did not match the expected result. Players keep their own models.'; continue }
            throw 'Generated extra mismatch. Nothing installed.'
        }
        $simonBuilt=$extraBytes
    } else {
        if ($extra.copyFrom -cne 'models/player.mdl' -and $extra.copyFrom -cne 'maps/c_difficulty_settings.bsp') { throw 'Invalid extra copy source.' }
        $copySource=Join-Path $gameRoot ($extra.copyFrom.Replace('/','\'))
        if (-not [IO.File]::Exists($copySource) -or (Get-PatchFileSha $copySource) -cne $extra.sha256) {
            if ($extra.optional -and $extra.copyFrom -ceq 'maps/c_difficulty_settings.bsp') { Write-Output ('Create Server entry skipped ('+$extra.target+'): maps\c_difficulty_settings.bsp is missing or a different version.'); continue }
            if ($extra.optional) { Write-Output 'Simon player model skipped: models\player.mdl is missing or a different version. Players keep their own models.'; continue }
            throw 'Required extra source missing or changed. Nothing installed.'
        }
        $extraBytes=[IO.File]::ReadAllBytes($copySource)
    }
    if ((Get-PatchSha $extraBytes) -cne $extra.sha256 -or $extraBytes.Length -ne $extra.fileBytes) { throw 'Changed extra artifact. Nothing installed.' }
    if ($extra.mutable -and $extra.target -ceq 'sc_setup.scr' -and $setupReplace) { $prepared.Add(@{path=$extraPath; exists=$false; replace=($extraPath+'.storycoop-old'); originalBytes=[IO.File]::ReadAllBytes($extraPath); artifactBytes=$extraBytes; fileBytes=$extraBytes.Length; extra=$true}); continue }
    $prepared.Add(@{path=$extraPath; exists=$extraExists; artifactBytes=$extraBytes; fileBytes=$extraBytes.Length; extra=$true})
}
$menuPath=Join-Path $maps '..\resource\GameMenu.res'
$menuBackup=$menuPath+'.storycoop-rc1-backup'
$menuPrepared=$null
$menuExists=[IO.File]::Exists($menuPath)
if ($Remove -and $null -ne $catalog.menu) {
    if ([IO.File]::Exists($menuBackup)) {
        if ((Get-PatchFileSha $menuBackup) -cne $catalog.menu.sourceSha256 -or -not $menuExists -or (Get-PatchFileSha $menuPath) -cne $catalog.menu.outputSha256) { throw 'Unknown/changed menu or backup retained. Nothing removed.' }
        $menuPrepared=[IO.File]::ReadAllBytes($menuBackup)
    }
} elseif (-not $Remove) {
    $dll=Join-Path $maps '..\cl_dlls\hl.dll'
    if (-not [IO.File]::Exists($dll) -or (Get-PatchFileSha $dll) -cne $catalog.nativeDllSha256 -or $catalog.nativeDllSha256 -cne '0036b91c01e92ed205513f52563053a55a66a32d257efb5f476b8f1f3dde0e63') { throw 'Native Steam DLL version mismatch. Nothing installed.' }
    if (-not $SkipMenu -and $null -ne $catalog.menu) {
        if (-not $menuExists) { throw 'Native GameMenu.res missing. Use -SkipMenu for map-only installation.' }
        $menuHash=Get-PatchFileSha $menuPath
        if ($menuHash -ceq $catalog.menu.outputSha256) {
            if (-not [IO.File]::Exists($menuBackup) -or (Get-PatchFileSha $menuBackup) -cne $catalog.menu.sourceSha256) { throw 'Patched menu has no verified original backup.' }
        } elseif ($menuHash -ceq $catalog.menu.sourceSha256) {
            if ([IO.File]::Exists($menuBackup) -and (Get-PatchFileSha $menuBackup) -cne $catalog.menu.sourceSha256) { throw 'Unknown menu backup retained.' }
            $menuBytes=[IO.File]::ReadAllBytes($menuPath)
            $latin1=[Text.Encoding]::GetEncoding(28591); $menuText=$latin1.GetString($menuBytes)
            $lastBrace=$menuText.LastIndexOf('}')
            if ($lastBrace -lt 0) { throw 'Invalid native menu.' }
            $menuPrepared=$latin1.GetBytes($menuText.Insert($lastBrace,$catalog.menu.insertText))
            if ((Get-PatchSha $menuPrepared) -cne $catalog.menu.outputSha256) { throw 'Generated menu mismatch.' }
        } else { throw 'Unknown/changed native menu retained. Use -SkipMenu for map-only installation.' }
    }
}
if ($VerifyOnly) { Write-Output "Verified $($prepared.Count) complete BSP plans and selected menu operation. No writes."; return }
if (-not $Remove) {
    [long]$needed=0; foreach ($item in $prepared) { if (-not $item.exists) { if ($item.ContainsKey('artifactBytes')) { $needed+=[long]$item.fileBytes } else { $needed+=[long]$item.plan.fileBytes } } }
    $drive=New-Object IO.DriveInfo ([IO.Path]::GetPathRoot($maps))
    if ($drive.IsReady -and $drive.AvailableFreeSpace -lt $needed) { throw "Insufficient disk space: need $needed bytes. Nothing installed." }
}
foreach ($item in $prepared) {
    if ($Remove -and $item.ContainsKey('restore')) {
        $restoreTemp=$item.path+'.storycoop-restore-'+[Guid]::NewGuid().ToString('N')+'.tmp'
        Write-NewPatchFile $restoreTemp $item.restoreBytes
        try { [IO.File]::Replace($restoreTemp,$item.path,[NullString]::Value) } finally { if ([IO.File]::Exists($restoreTemp)) { [IO.File]::Delete($restoreTemp) } }
        [IO.File]::Delete($item.restore); Write-Output "Restored the original $($item.path)"
    }
    elseif ($Remove) {
        if ($item.exists) { [IO.File]::Delete($item.path); Write-Output "Removed $($item.path)" }
        # The Simon model folder was created by this installer; remove it once empty.
        if ($item.ContainsKey('extra')) { $folder=[IO.Path]::GetDirectoryName($item.path); if ([IO.Path]::GetFileName($folder) -ceq 'simon' -and [IO.Directory]::Exists($folder) -and @([IO.Directory]::GetFileSystemEntries($folder)).Count -eq 0) { [IO.Directory]::Delete($folder) } }
    }
    elseif ($item.exists) { Write-Output "Already installed and verified: $($item.path)" }
    elseif ($item.ContainsKey('replace')) {
        if (-not [IO.File]::Exists($item.replace)) { Write-NewPatchFile $item.replace $item.originalBytes }
        $replaceTemp=$item.path+'.storycoop-new-'+[Guid]::NewGuid().ToString('N')+'.tmp'
        Write-NewPatchFile $replaceTemp $item.artifactBytes
        try { [IO.File]::Replace($replaceTemp,$item.path,[NullString]::Value) } finally { if ([IO.File]::Exists($replaceTemp)) { [IO.File]::Delete($replaceTemp) } }
        Write-Output "Installed $($item.path) (original kept as $([IO.Path]::GetFileName($item.replace)))"
    }
    else {
        if ($item.ContainsKey('extra')) { [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($item.path)) }
        if ($item.ContainsKey('artifactBytes')) { $bytes=$item.artifactBytes } else { $bytes=New-PatchedBsp ([IO.File]::ReadAllBytes($item.source)) $item.plan }; Write-NewPatchFile $item.path $bytes; $bytes=$null; Write-Output "Installed $($item.path)"
    }
}
# v0.5.2 (Katz 2026-10-23: "chapter 02 is higher than prologue"): the Create
# Server window lists its entries in the order the folder gives them. NTFS
# sorts names; exFAT and FAT keep the order the files were written (the
# tester's D: drive is exFAT and listed Chapter-2 first). Whenever the folder
# lists them out of the catalog's order, the entries are written again in order
# (exFAT puts each new entry in the first free place it fits); the result is checked.
if (-not $Remove) {
    $entryNames=@($catalog.extras | Where-Object { $null -ne $_ -and $_.target -cmatch '^maps/(Chapter-[0-7](-[A-Za-z]+)+|Load-Save-Slot-[1-5])\.bsp$' } | ForEach-Object { [IO.Path]::GetFileName($_.target) })
    $entryPaths=@($entryNames | ForEach-Object { Join-Path $maps $_ })
    if ($entryNames.Count -and @($entryPaths | Where-Object { -not [IO.File]::Exists($_) }).Count -eq 0) {
        $listed=@([IO.Directory]::EnumerateFiles($maps,'*-*bsp') | ForEach-Object { [IO.Path]::GetFileName($_) } | Where-Object { $entryNames -contains $_ })
        if (($listed -join '|') -cne ($entryNames -join '|')) {
            $entryBytes=New-Object 'System.Collections.Generic.List[byte[]]'
            foreach ($p in $entryPaths) { $entryBytes.Add([IO.File]::ReadAllBytes($p)) }
            foreach ($p in $entryPaths) { [IO.File]::Delete($p) }
            # Written directly: a temporary name renamed afterwards takes a different directory slot.
            for ($i=0; $i -lt $entryPaths.Count; $i++) { [IO.File]::WriteAllBytes($entryPaths[$i],$entryBytes[$i]) }
            $listed=@([IO.Directory]::EnumerateFiles($maps,'*-*bsp') | ForEach-Object { [IO.Path]::GetFileName($_) } | Where-Object { $entryNames -contains $_ })
            if (($listed -join '|') -ceq ($entryNames -join '|')) { Write-Output 'Create Server entries rewritten in chapter order.' } else { Write-Output 'Create Server entries rewritten, but this drive still lists them out of order.' }
        }
    }
}
if ($null -ne $menuPrepared) {
    if ($Remove) {
        $restoreTemp=$menuPath+'.storycoop-restore-'+[Guid]::NewGuid().ToString('N')+'.tmp'
        Write-NewPatchFile $restoreTemp $menuPrepared
        try { [IO.File]::Replace($restoreTemp,$menuPath,[NullString]::Value) } finally { if ([IO.File]::Exists($restoreTemp)) { [IO.File]::Delete($restoreTemp) } }
        [IO.File]::Delete($menuBackup); Write-Output 'Restored original native menu.'
    } else {
        if (-not [IO.File]::Exists($menuBackup)) { Write-NewPatchFile $menuBackup ([IO.File]::ReadAllBytes($menuPath)) }
        $menuTemp=$menuPath+'.storycoop-menu-'+[Guid]::NewGuid().ToString('N')+'.tmp'
        Write-NewPatchFile $menuTemp $menuPrepared
        try { [IO.File]::Replace($menuTemp,$menuPath,[NullString]::Value) } finally { if ([IO.File]::Exists($menuTemp)) { [IO.File]::Delete($menuTemp) } }
        Write-Output 'Added Host Coop Game; existing menu entries retained.'
    }
}
if (-not $Remove) { Write-Output 'EXPERIMENTAL: use native Steam co-op initialization. Follow TESTING.md. Gameplay verification is required.' }
if (-not $Remove -and $catalog.release -eq 'connected-story-campaign-rc2') { Write-Output "RC2 is incomplete: $($catalog.summary.blockedEntries.Count) blocked arrivals and $($catalog.summary.remainingDestinationDoors) retained destination doors. Start only through the supplied Steam launcher or native host dialog; see TESTING.md." }
if (-not $Remove -and $catalog.release -eq 'story-opening-rc3') { Write-Output 'RC3 opening candidate installed. Host cof_campaign_story3 with exactly two players. Runtime fixes and nightmare handoff are awaiting gameplay verification. RC2 is retained.' }
if (-not $Remove -and $null -ne $catalog.extras) { Write-Output 'Main menu: start Cry of Fear with "Run Co-op Mod.cmd" (H = host), then "Host Simon''s Story" > "Simon''s Story: New Game" or "Simon''s Story: Load Slot 1/2/3".' }
if (-not $Remove -and $catalog.release -eq 'story-state-rc4') { Write-Output 'RC4 experimental adapter and new aliases installed. Existing RC3 and original binaries retained. Use Host RC4.cmd; first confirm the adapter bootstrap log. No disk saves or gameplay pass claimed.' }
