#!/usr/bin/env python

import sys
import os
import re
import sqlite3

def scanDirectory(directory):
    job_files = {}
    regexFile = re.compile(
    r"MpaCmaesAlign_num([0-9]+)_([0-9]+)(_path\.csv|_space\.csv|\.status|\.config|\.cache)")
    for root, dirs, files in os.walk(directory):
        for f in files:
            m = regexFile.search(f)
            if m:
                job_id = int(m.group(1))
                run_id = int(m.group(2))
                filetype = m.group(3)
                if not job_id in job_files:
                    job_files[job_id] = {
                        "run_id": run_id,
                        "job_id": job_id,
                    }
                types = ["path", "space", "status", "config", "cache"]
                for t in types:
                    if t in filetype:
                        job_files[job_id][t+"_file"] = os.path.join(root, f)
    return job_files

def createDatabase(database_file):
    print "Database: '{}'".format(database_file)
    if os.path.exists(database_file):
        print "Database file exists! Exiting."
        sys.exit(1)
    conn = sqlite3.connect(database_file)
    c = conn.cursor()
    c.execute("""CREATE TABLE `jobs` (job_id INTEGER PRIMARY_KEY, run_id INTEGER,
            exit_status INTEGER);""")
    
    c.execute("""CREATE TABLE config_float (job_id INTEGER, key STRING,
    value FLOAT,
    PRIMARY KEY (job_id, key));""")
    
    c.execute("""CREATE TABLE config_int (job_id INTEGER, key STRING,
    value INTEGER,
    PRIMARY KEY (job_id, key));""")
    
    c.execute("""
CREATE TABLE path (
 id INTEGER PRIMARY KEY,
 job_id INTEGER,
 generation int,
 x float,
 y float,
 z float,
 phi float,
 theta float,
 omega float,
 fitness float,
 sigma float
);
""")
    
    c.execute("""
CREATE TABLE parameterspace (
 id INTEGER PRIMARY KEY,
 job_id INTEGER,
 feval_no int,
 x float,
 y float,
 z float,
 phi float,
 theta float,
 omega float,
 fitness float,
 total_tracks float,
 used_tracks float
);
""")

    c.execute("""
CREATE TABLE intermediate_status (
 id INTEGER PRIMARY KEY,
 job_id INTEGER,
 iteration INTEGER,
 status INTEGER
);
""")
    
    c.execute("""
CREATE TABLE hitpairs (
 id INTEGER PRIMARY KEY,
 run_id INTEGER,
 mpa_index INTEGER,
 ax float,
 ay float,
 az float,
 bx float,
 by float,
 bz float
);
""")
    
    c.execute("""
CREATE TABLE finals (
 id INTEGER PRIMARY KEY,
 job_id INTEGER,
 generation int,
 x float,
 y float,
 z float,
 phi float,
 theta float,
 omega float,
 fitness float,
 sigma float
);
""")
    conn.commit();
    return conn

def createIndex(conn):
    print "Create indices..."
    c = conn.cursor()
    c.execute("CREATE INDEX `index_jobs_run_id` ON `jobs` (run_id);")
    c.execute("CREATE INDEX `index_jobs_exit_status` ON `jobs` (exit_status);")
    c.execute("CREATE INDEX index_config_float_value ON config_float(value);")
    c.execute("CREATE INDEX index_config_int_value ON config_int(value);")
    c.execute("CREATE INDEX index_path_job_id ON path(job_id);")
    c.execute("CREATE INDEX index_parameterspace_fitness ON "
              "parameterspace(fitness);")
    c.execute("CREATE INDEX index_intermediate_status_job_id ON "
              "intermediate_status(job_id);")
    c.execute("CREATE INDEX index_hitpairs_run_id ON hitpairs(run_id);")
    c.execute("CREATE INDEX index_finals_job_id ON finals(job_id);")
    print "Commit changes..."
    conn.commit();
    print "... done!"

def readCsvFile(filename, datatype=float):
    data = []
    with open(filename) as f:
        for line in f:
            line = line.strip()
            if line.startswith("#"):
                continue
            data.append([datatype(x) for x in line.split()])
    return data


def writeJobToDatabase(c, job, write_cache):
    job_id = job["job_id"]
    run_id = job["run_id"]
    status_list = readCsvFile(job["status_file"], int)
    exit_status = status_list[-1][1]
    for idx, _ in enumerate(status_list):
        status_list[idx].append(job_id)
    c.executemany("""
INSERT INTO intermediate_status ( iteration, status, job_id ) VALUES(?,?,?)""",
status_list)
    c.execute("INSERT INTO jobs VALUES (?, ?, ?)", (job_id, run_id,
           exit_status))
    config_list = readCsvFile(job["config_file"], lambda x: x)
    config_int = []
    config_float = []
    for cfg in config_list:
        if cfg[0] == "int":
            config_int.append([job_id, cfg[1], int(cfg[2])])
        else:
            config_float.append([job_id, cfg[1], float(cfg[2])])
    c.executemany("INSERT INTO config_int VALUES(?,?,?)", config_int)
    c.executemany("INSERT INTO config_float VALUES(?,?,?)", config_float)
    path_list = readCsvFile(job["path_file"])
    for idx, _ in enumerate(path_list):
        path_list[idx].append(job_id)
        path_list[idx].append(idx)
    c.executemany("""
INSERT INTO path ( fitness, x, y, z, phi, theta, omega, sigma, job_id, generation ) VALUES (?,?,?,?,?,?,?,?,?,?);
""", path_list)
    try:
        space_list = readCsvFile(job["space_file"])
        for idx, _ in enumerate(space_list):
            space_list[idx].append(job_id)
            space_list[idx].append(idx)
        c.executemany("""
INSERT INTO parameterspace ( fitness, x, y, z, phi, theta, omega, total_tracks,
used_tracks, job_id, feval_no ) VALUES (?,?,?,?,?,?,?,?,?,?,?);
""", space_list)
    except KeyError:
        pass
    if write_cache:
        cache_list = readCsvFile(job["cache_file"])
        for idx, _ in enumerate(cache_list):
            cache_list[idx].append(run_id)
        c.executemany("""
    INSERT INTO hitpairs ( mpa_index, ax, ay, az, bx, by, bz, run_id ) VALUES
    (?,?,?,?,?,?,?,?)
    """, cache_list)
    finals_list = path_list[-1]
    finals_list.pop()
    c.execute("""
INSERT INTO finals ( fitness, x, y, z, phi, theta, omega, sigma, job_id ) VALUES (?,?,?,?,?,?,?,?,?);
""", finals_list)

def main():
    if len(sys.argv) != 3:
        print "Usage: {} DATA_PATH OUTPUT_DATABASE".format(sys.argv[0])
        sys.exit(1)
    db = createDatabase(sys.argv[2])
    job_files = scanDirectory(sys.argv[1])
    job_no = 0
    for job_no, job in enumerate(job_files.itervalues()):
        sys.stdout.write("\r\x1b[1KConverting job {} of {}".format(job_no+1,
            len(job_files)))
        sys.stdout.flush()
        run_id = job["run_id"]
        with db:
            c = db.cursor()
            c.execute(
            "SELECT count(id) FROM hitpairs WHERE run_id = ? LIMIT 1", (run_id,))
            write_cache = c.fetchone()[0] == 0
        try:
            with db:
                writeJobToDatabase(db, job, write_cache)
            db.commit()
        except Exception as e:
            print "Got error: {}\nIgnoring.".format(str(e))
    sys.stdout.write("\r\x1b[1KConversion finished!\n")
    sys.stdout.flush()
    createIndex(db)
if __name__ == "__main__":
    main()

