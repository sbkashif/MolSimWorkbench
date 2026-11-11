# Write one frame of the cover animation as a Tachyon scene file.
#   echo 'source render_frames.tcl' > run.tcl
#   vmd -dispdev text -e run.tcl -args <index>
#   tachyon -res 1000 1000 -aasamples 8 -format TARGA \
#           -o frames/f<index>.tga frames/f<index>.dat
# Reads split/s<index>.xyz. Frame 0 also saves the camera to view.txt; every
# later frame loads it, so the view never moves.
# VMD 2.0a7 text mode quirks: run the script through `source` (with -e
# directly it stops early), use one VMD process per frame (several renders in
# one session come out blank), and do not `display resize` (blank again); the
# external tachyon binary sets the resolution instead.
package require pbctools
set L [expr {pow(512/0.8, 1.0/3.0)}]
set i [lindex $argv 0]

display projection Orthographic
display depthcue off
display ambientocclusion on
display shadows on
color Display Background white
axes location off

set m [mol new [format "split/s%03d.xyz" $i] type xyz waitfor all autobonds off]
mol delrep 0 $m
mol representation VDW 0.42 30
mol color ColorID 10
mol material AOChalky
mol addrep $m
pbc set [list $L $L $L] -molid $m
pbc box -center origin -color gray -width 2 -molid $m

set keys {center_matrix rotate_matrix scale_matrix global_matrix}
if {$i == 0} {
  display resetview
  rotate y by 30
  rotate x by 20
  scale by 0.9
  set f [open view.txt w]
  puts $f [molinfo $m get $keys]
  close $f
} else {
  set f [open view.txt r]
  molinfo $m set $keys [read $f]
  close $f
}
render Tachyon [format "frames/f%03d.dat" $i]
quit
