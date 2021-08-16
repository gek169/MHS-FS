# MHS Filesystem

The MHS filesystem. Features:

* can be modified to work with any size of disk or sector, even non powers of two!
* Allocation bitmap stored on-disk for improved boot times.
* Recoverability- if the power shuts off during a write, as long as a *sector* write was atomic (I.E. not interrupted *during* a sector write)
then the system will recover just fine. However, the allocation bitmap may have to be re-generated (SLOW)
* Very easy to implement. Much simpler than Fat32.

Any disk which can be abstracted into a linear series of sectors or blocks can be made to work with this filesystem.
