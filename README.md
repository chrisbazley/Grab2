# Grab2

System sprite grabber

© Chris Bazley, 2002

Version 1.19 (24 Sep 2015)

-----------------------------------------------------------------------------
1   Introduction and Purpose
============================

The short intro:

  This program allows the contents of the Window Manager's sprite pool, or
the window tool sprites, to be grabbed as a sprite file.

The long intro:

  A long time ago in a galaxy not very far away, there was a computer
program called "Grab". The writings of the wise prophets of ancient Babylon
record that its purpose was that of "System Sprite ripper". By reciting the
appropriate incantations over this artefact, the high priests could save the
RISC OS System Sprite pool (ROM or RAM) as a Sprite file to be used for
whatever they wished. The creator of this mighty software giant of the
ancient world was none other than the legendary Dave Thomas, who inscribed
the words "by David Thomas, © 1992" upon his finished work.

  Approaching its tenth anniversary, however, "Grab" was found to be looking
all wrinkly and old in the sparkling new candy-blossom funland of the RISC
OS 4 desktop. Its text icons were too narrow for anything but system font,
its OS 2 vintage window sprites looked nasty and blocky, and its distinctly
non-style-guide user interface was conjured up by the weird and ancient
'InterfaceManager' module.

  So, I nicked the application sprites and wrote this replacement "Grab II",
from scratch in a few hours. Partly to prove how easy it is to knock
together applications using Acorn's Toolbox system, but also because once
too often I had tried to drag sprites from the old "Grab" straight into
Paint, forgetting that this trivial act was beyond its capabilities.

Improvements:
   Supports interactive help.
   Direct data transfer to other applications.
   Style-guide compliant user interface.
   Support for international resources (just translate messages).
   Doesn't rely on weird and ancient InterfaceManager module.
   May also grab window tool icon sprites.

Non-improvements:
   Requires a monstrous 36K memory to run instead of 16K.

-----------------------------------------------------------------------------
2   Requirements
================

- RISC OS 3.10 or better (OS 3.5 to grab window tools)
- The Toolbox modules (in !System, or in ROM with RISC OS 3.6)

  If any of the above facilities are unavailable then the program will fail
to load, with an error message describing the problem.

-----------------------------------------------------------------------------
3   Quick Guide
===============

 After you have loaded Grab II, its icon will appear on the icon bar.
Clicking SELECT over the icon bar icon will open its main window. This main
window contains the same components as a standard save box plus a couple of
others.

  To save you must first select whether you want to save the Wimp sprite
pool (ROM or RAM area) or the tool sprites used for window borders. You
should do this by clicking on the relevant radio button. Then you must enter
a name for the file by typing it into the writable icon at the bottom of the
box.

  After doing this you can drag the Sprite icon to a filer window and (hey
presto!) your sprites will be saved. These sprites can then be loaded into
Paint for fiddling about with.

  Note that under RISC OS 3 or 4 you can also find the Window Manager's ROM
tool/icon sprites in ResourceFS as "Resources:$.Resources.Wimp.Sprites",
"Resources:$.Resources.Wimp.Sprites22" and "Resources:$.Resources.Wimp.
Tools". If you just want the default sprite sets (rather than the current
sprite pool) then copying these files may be simpler.

-----------------------------------------------------------------------------
4   Technical details
=====================

  SWI "Wimp_BaseOfSprites" is used to get the current addresses of the
Window Manager's ROM and RAM sprite areas.

  SWI "Wimp_ReadSysInfo" 9 is used to get the address of the toolsprites
area control block. This reason code is only available from RISC OS 3.5
onwards, so if an earlier version of the Window Manager is detected then the
"Toolsprites" radio button will be greyed out.

  Instead of simply using SWI "OS_SpriteOp" 12 to save Wimp sprite areas
to a file, custom-written code is used. This is required to take account of
the fact that the sprite data may not be contiguous with the sprite area
header and (at least on RISC OS 6) some sprites in the Wimp pool have no
name. Invalid sprites are not saved. After all of the valid sprites have
been saved, a suitable header is prepended to the file.

-----------------------------------------------------------------------------
5   Program history
===================

1.00 (5th May 2002)
   First version.

1.10 (19th August 2002)
   Now allows window tool sprites to be grabbed, on RISC OS 3.5 or above.

1.11 (10th November 2002)
   Recompiled using the official Castle release of 32-bit Acorn C/C++.
   Corrected mistake in !Run file where the CallASWI module was not being
    loaded on RISC OS 3.6, despite being required on all OSes prior to 3.7.

1.12 (19th May 2003)
   More sensible response to error on Toolbox_Initialise (doesn't rely on
    messages file, which isn't open).
   Now resets save dialogue box when cancel button is clicked.

1.13 (27th June 2003)
   Now broadcasts Message_WindowClosed before opening the savebox, on the
    off-chance that the window has been iconised (though you would need an
    external iconiser to do this).

1.14 (9th September 2003)
   Re-released as open source software under the GNU General Public Licence.
   Formatted the manual text for a fixed-width 77 column display (Zap's
    default).
   !Run file now requires version 5.43 of the SharedCLibrary, since this is
    the earliest version documented as supporting C99 functions.
   Now uses "NoMem" from the global messages file rather than own error
    message.

Version 1.15 (2nd March 2004)
   Re-compiled with release 9 of CBLibrary.

Version 1.16 (26th September 2004)
   Moved to new-style CBLibrary macro names.
   Corrected section numbering in text manual (were two section 3s!)
   Dialogue box is now transient & coloured appropriately.
   Removed all code dedicated to keeping track of the last filename used
    and restoring it upon an ADJUST-click on 'Cancel' - this is really the
    responsibility of the SaveAs module, which may eventually be fixed in
    this respect (as the Scale module has been already).

Version 1.17 (16 Sep 2005)
   Updated ResFind from version 2.01b to latest version 2.12 (improved
    support for truncated country names & long paths).
   No longer requires the new (C99, APCS-32) shared C library or a version
    of the floating point emulator that supports LFM/SFM instructions.

Version 1.18 (05 Jul 2010)
   Specified the appropriate switches to prevent the Norcroft C compiler
    from generating unaligned memory accesses incompatible with ARM v6 and
    later (for RISC OS on BeagleBoard).
   Added *WimpSlot commands to ensure that there is sufficient memory in
    which to run the ResFind utility.
   Added a mechanism to set country-specific system variables for Castle's
    help system (Grab2$Description, Grab2$Publisher, Grab2$Title, etc).
   Defined several 'magic' values as named constants.
   Anonymous enumerations are now used to define numeric constants in
    preference to macro values.
   Now uses standard C functions when saving tool sprites instead of
    Acorn's library kernel functions.
   Utilises the set_gadget_faded function introduced in release 32 of
    CBLibrary.
   Utilises the set_file_type function introduced in release 33 of
    CBLibrary.
   No longer uses function and macro names deprecated in release 34 of
    CBLibrary.
   Removed the automatically-generated list of dynamic dependencies from
    the Makefile.

Version 1.19 (24 Sep 2015)
   Fixed a bug where tool sprites were saved in an invalid sprite file
    with an extra word of data at the end. The correct data size cannot be
    obtained by subtracting the offset to the first sprite from the offset
    to the first free word, as in a normal sprite area. Earlier versions of
    Grab incorporated a fudge factor to solve that problem but Grab 1.18
    did not and was therefore broken.
   SWI "OS_SpriteOp" 12 is no longer used to save any of the Wimp's sprite
    areas because they are too esoteric. In particular, RISC OS 6 has zero-
    named sprites in its Wimp pool and its tool sprite area's header is at
    a higher address than the rest of the data.

-----------------------------------------------------------------------------
6   Compiling the program
==========================

  If you did not receive source code with this program then you can download
it from http://starfighter.acornarcade.com/mysite/ To compile and link the code
you will also require CBLibrary, ISO 'C' library headers, stubs for the
shared C library and the flex, toolbox, event & wimp libraries supplied with
the Acorn C/C++ Development Suite. CBLibrary is available separately from
the same web site as above.

  The make file is currently set up to compile code compatible with floating
point emulator 2 and link against RISCOS Ltd's generic stubs library
('StubsG').

-----------------------------------------------------------------------------
7   Licence and Disclaimer
==========================

  This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public Licence as published by the Free
Software Foundation; either version 2 of the Licence, or (at your option)
any later version.

  This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public Licence for
more details.

  You should have received a copy of the GNU General Public Licence along
with this program; if not, write to the Free Software Foundation, Inc.,
675 Mass Ave, Cambridge, MA 02139, USA.

-----------------------------------------------------------------------------
8   Credits
===========

  Grab II was designed and programmed by Chris Bazley.

  This program uses CBLibrary, which is (C) 2003 Chris Bazley. This library
and its use are covered by the GNU Lesser General Public Licence. You can
obtain CBLibrary with source code by downloading it from my web site (see
'Contact details').

  Olaf Krumnow and Herbert zur Nedden of the German Archimedes Group wrote
the ResFind program I use to find the correct resources for different
languages.

  The application name "Grab2" (including associated sprites/system
variables) has been officially registered with RISCOS Ltd.

  Original 'Grab' concept and application icons by David Thomas, © 1992.

-----------------------------------------------------------------------------
9  Contact details
==================

Feel free to contact me with any bug reports, suggestions or anything else.

  Email: mailto:cs99cjb@gmail.com

  WWW:   http://starfighter.acornarcade.com/mysite/
