/*
 * snapshotobj.c
 *
 * Copyright (c) 2017 The MacPorts Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <tcl.h>
#include <sqlite3.h>

#include "snapshotobj.h"
#include "registry.h"
#include "util.h"

const char* snapshot_props[] = {
    "note",
    "ports",
    "created_at",
    NULL
};

/* ${snapshot} prop */
static int snapshot_obj_prop(Tcl_Interp* interp, reg_snapshot* snapshot, int objc,
        Tcl_Obj* CONST objv[]) {
    int index;
    if (objc > 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "?value?");
        return TCL_ERROR;
    }
    if (objc == 2) {
        /* ${snapshot} prop; return the current value */
        reg_registry* reg = registry_for(interp, reg_attached);
        if (reg == NULL) {
            return TCL_ERROR;
        }
        if (Tcl_GetIndexFromObj(interp, objv[1], snapshot_props, "prop", 0, &index)
                == TCL_OK) {
            char* key = Tcl_GetString(objv[1]);
            char* value;
            reg_error error;
            if (reg_snapshot_propget(snapshot, key, &value, &error)) {
                Tcl_Obj* result = Tcl_NewStringObj(value, -1);
                Tcl_SetObjResult(interp, result);
                free(value);
                return TCL_OK;
            }
            return registry_failed(interp, &error);
        }
        return TCL_ERROR;
    }
}

/* ${snapshot} ports */
static int snapshot_obj_ports(Tcl_Interp* interp, reg_snapshot* snapshot, int objc,
        Tcl_Obj* CONST objv[]) {
    int index;
    if (objc > 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "?value?");
        return TCL_ERROR;
    }
    if (objc == 2) {
        /* ${snapshot} prop; return the current value */
        reg_registry* reg = registry_for(interp, reg_attached);
        if (reg == NULL) {
            return TCL_ERROR;
        }
        if (Tcl_GetIndexFromObj(interp, objv[1], snapshot_props, "prop", 0, &index)
                == TCL_OK) {
            port** ports;
            reg_error error;
            if (reg_snapshot_ports_get(snapshot, &ports, &error)) {

                // TODO: correct the below for 'ports', added as a prototype for now
                // Tcl_Obj** objs;
                // int retval = TCL_ERROR;
                // if (list_entry_to_obj(interp, &objs, entries, entry_count, &error)){
                //     Tcl_Obj* result = Tcl_NewListObj(entry_count, objs);
                //     Tcl_SetObjResult(interp, result);
                //     free(objs);
                //     retval = TCL_OK;
                // } else {
                //     retval = registry_failed(interp, &error);
                // }

                free(ports);
                return TCL_OK;
            }
            return registry_failed(interp, &error);
        }
        return TCL_ERROR;
    }
}

typedef struct {
    char* name;
    int (*function)(Tcl_Interp* interp, reg_snapshot* snapshot, int objc,
            Tcl_Obj* CONST objv[]);
} snapshot_obj_cmd_type;

static snapshot_obj_cmd_type snapshot_cmds[] = {
    /* keys */
    { "note", snapshot_obj_prop },
    { "created_at", snapshot_obj_prop },
    /* ports */
    { "ports", snapshot_obj_ports },
    { NULL, NULL }
};

/* ${snapshot} cmd ?arg ...? */
/* This function implements the command that will be called when a snapshot
 * created by `registry::snapshot` is used as a procedure. Since all data is kept
 * in a temporary sqlite3 database that is created for the current interpreter,
 * none of the sqlite3 functions used have any error checking. That should be a
 * safe assumption, since nothing outside of registry:: should ever have the
 * chance to touch it.
 */
int snapshot_obj_cmd(ClientData clientData, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[]) {
    int cmd_index;
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "cmd ?arg ...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[1], snapshot_cmds,
                sizeof(snapshot_obj_cmd_type), "cmd", 0, &cmd_index) == TCL_OK) {
        snapshot_obj_cmd_type* cmd = &snapshot_cmds[cmd_index];
        return cmd->function(interp, (reg_snapshot*)clientData, objc, objv);
    }
    return TCL_ERROR;
}