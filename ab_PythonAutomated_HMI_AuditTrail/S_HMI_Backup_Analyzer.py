import os
import shutil 
import csv
from datetime import datetime

# Create a backup of an HMI audit trail file
def backup_SAudit(src_path, backup_root="audit_bu"):
    if not os.path.exists(src_path):
        raise FileNotFoundError("HMI audit trail file not found")
    
    os.makedirs(backup_root, exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d_%H_%M_%S")
    filename = os.path.basename(src_path)
    backup_path = os.path.join(backup_root, f"{filename}_{timestamp}")

    shutil.copy2(src_path, backup_path)

    return backup_path

# Structure the events recorded of a typical Siemens HMI audit csv file
def parts_SAudit(audit_csv):
    
    '''Date,Time,User,Action,Object,Old value,New value
    2025-11-28,14:35:02,Engineer,Parameter change,Speed,120,150
    2025-11-28,14:40:18,Operator,Logout'''

    events = []

    with open(audit_csv, newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)

        for row in reader:
            timestamp = datetime.strptime(f"{row['Date']} {row['Time']}","%Y-%m-%d %H:%M:%S")
            
            event = {"timestamp": timestamp , "user": row.get("User", "").strip(), 
                "action": row.get("Action", "").strip(), "object": row.get("Object", "").strip(), 
                "old": row.get("Old value", "").strip(), "new": row.get("New value", "").strip()}
            
            events.append(event)

    return events

# Analyze the structured audit events and extract the 'main' events
def analyze_SAudit(events):
    report = {}

    report["all_events"] = len(events)
    report["engineer_access"] = [e for e in events if e["user"].lower().startwith("eng")]
    report["parameter_changes"] = [e for e in events if "parameter" in e["action"].lower()]

    return report

# Create a txt report file with the key events
def write_SAudit_report(data, output="System1_audit_report.txt"):
    with open(output, "w") as f:
        f.write("\t\tSYSTEM N°1 HMI AUDIT REPORT\n")
        f.write(datetime.now().strftime("%d-%m-%Y %H:%M:%S"))
        f.write("\n============================================================\n\n")
        f.write(f"Total events: {data['total_events']}\n")
        f.write(f"Parameter changes: {len(data['parameter_changes'])}\n")
        f.write(f"Engineering access events: {len(data['engineering_access'])}\n")


''' Exemple : 
    backup_AUfile = backup_SAudit(r"\\192.168.0.10\Audit\AuditTrail.csv")
    events = parts_SAudit(backup_AUfile)
    report_data = analyze_SAudit(events)
    write_SAudit_report(report_data)    '''