# Welcome to Harbour

Harbour is the open/free software implementation of a cross-platform,
multi-threading, object-oriented, scriptable programming language, backwards
compatible with xBase languages. Harbour consists of a compiler and runtime
libraries with multiple UI, database and I/O backends, its own build system
and a collection of libraries and bindings for popular APIs.

## my fork (hernad)

sddpg patch which provides using one connection for SQLMIX and hbpgsql access to PostgreSQL database


<pre>
PROCEDURE show_postgresql_version( hParams )

LOCAL oServer, pConn

oServer := TPQServer():New( hParams[ "host" ], hParams[ "database" ] , hParams[ "user" ] , hParams[ "password" ] )

pConn := oServer:pDB

rddSetDefault( "SQLMIX" )

IF rddInfo( 1001, { "POSTGRESQL", pConn } ) == 0 // reusing pConn
      ? "Could not connect to the server"
      RETURN
ENDIF

// xBase interface to postgresql database
dbUseArea( .T., , "SELECT version() AS ver", "INFO" )
OutStd( field->ver )

RETURN
</pre>

