#!/usr/bin/python
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
# Copyright (c) 2012 Dominik Riebeling
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#

'''Scrape files from a git repository.

This module provides functions to get a subset of files from a git repository.
The files to retrieve can be specified, and the git tree to work on can be
specified. That was arbitrary trees can be retrieved (like a subset of files
for a given tag).

Retrieved files can be packaged into a bzip2 compressed tarball or stored in a
given folder for processing afterwards.

Calls git commands directly for maximum compatibility.
'''

import re
import subprocess
import os
import tarfile
import tempfile
import shutil


def get_refs(repo):
    '''Get dict matching refs to hashes from repository pointed to by repo.
    @param repo Path to repository root.
    @return Dict matching hashes to each ref.
    '''
    print "Getting list of refs"
    output = subprocess.Popen(["git", "show-ref"], stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, cwd=repo)
    cmdout = output.communicate()
    refs = {}

    if len(cmdout[1]) > 0:
        print "An error occured!\n"
        print cmdout[1]
        return refs

    for line in cmdout:
        regex = re.findall(r'([a-f0-9]+)\s+(\S+)', line)
        for r in regex:
            # ref is the key, hash its value.
            refs[r[1]] = r[0]

    return refs


def get_lstree(repo, start, filterlist=[]):
    '''Get recursive list of tree objects for a given tree.
    @param repo Path to repository root.
    @param start Hash identifying the tree.
    @param filterlist List of paths to retrieve objecs hashes for.
                      An empty list will retrieve all paths.
    @return Dict mapping filename to blob hash
    '''
    output = subprocess.Popen(["git", "ls-tree", "-r", start],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=repo)
    cmdout = output.communicate()
    objects = {}

    if len(cmdout[1]) > 0:
        print "An error occured!\n"
        print cmdout[1]
        return objects

    for line in cmdout[0].split('\n'):
        regex = re.findall(r'([0-9]+)\s+([a-z]+)\s+([0-9a-f]+)\s+(\S+)', line)
        for rf in regex:
            # filter
            add = False
            for f in filterlist:
                if rf[3].find(f) == 0:
                    add = True

            # If two files have the same content they have the same hash, so
            # the filename has to be used as key.
            if len(filterlist) == 0 or add == True:
                if rf[3] in objects:
                    print "FATAL: key already exists in dict!"
                    return {}
                objects[rf[3]] = rf[2]
    return objects


def get_object(repo, blob, destfile):
    '''Get an identified object from the repository.
    @param repo Path to repository root.
    @param blob hash for blob to retrieve.
    @param destfile filename for blob output.
    @return True if file was successfully written, False on error.
    '''
    output = subprocess.Popen(["git", "cat-file", "-p", blob],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=repo)
    cmdout = output.communicate()
    # make sure output path exists
    if len(cmdout[1]) > 0:
        print "An error occured!\n"
        print cmdout[1]
        return False
    if not os.path.exists(os.path.dirname(destfile)):
        os.makedirs(os.path.dirname(destfile))
    f = open(destfile, 'wb')
    for line in cmdout[0]:
        f.write(line)
    f.close()
    return True


def describe_treehash(repo, treehash):
    '''Retrieve output of git-describe for a given hash.
    @param repo Path to repository root.
    @param treehash Hash identifying the tree / commit to describe.
    @return Description string.
    '''
    output = subprocess.Popen(["git", "describe", treehash],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=repo)
    cmdout = output.communicate()
    if len(cmdout[1]) > 0:
        print "An error occured!\n"
        print cmdout[1]
        return ""
    return cmdout[0].rstrip()


def scrape_files(repo, treehash, filelist, dest=""):
    '''Scrape list of files from repository.
    @param repo Path to repository root.
    @param treehash Hash identifying the tree.
    @param filelist List of files to get from repository.
    @param dest Destination path for files. Files will get retrieved with full
                path from the repository, and the folder structure will get
                created below dest as necessary.
    @return Destination path.
    '''
    print "Scraping files from repository"

    if dest == "":
        dest = tempfile.mkdtemp()
    treeobjects = get_lstree(repo, treehash, filelist)
    for obj in treeobjects:
        get_object(repo, treeobjects[obj], os.path.join(dest, obj))

    return dest


def archive_files(repo, treehash, filelist, basename, tmpfolder=""):
    '''Archive list of files into tarball.
    @param repo Path to repository root.
    @param treehash Hash identifying the tree.
    @param filelist List of files to archive. All files in the archive if left
                    empty.
    @param basename Basename (including path) of output file. Will get used as
                    basename inside of the archive as well (i.e. no tarbomb).
    @param tmpfolder Folder to put intermediate files in. If no folder is given
                     a temporary one will get used.
    @return Output filename.
    '''
    print "Archiving files from repository"

    workfolder = scrape_files(repo, treehash, filelist, tmpfolder)
    outfile = basename + ".tar.bz2"
    tf = tarfile.open(outfile, "w:bz2")
    tf.add(workfolder, basename)
    tf.close()
    if tmpfolder != workfolder:
        shutil.rmtree(workfolder)
    return outfile
