#!/usr/bin/python3
import json
import logging
import sys
logger = logging.getLogger()
logger.setLevel(logging.ERROR)

def insert_before(d, key, item):
    items = list(d.items())
    items.insert(list(d.keys()).index(key), item)
    return dict(items)

def update_performance_numbers(performance_json_path, performance_json, new_json_numbers):
    for key, items in new_json_numbers.items():
        if len(key) == 0:
            continue

        if not key in performance_json["Instructions"]:
            logging.error("{} didn't exist in performance json file?".format(key))
            return 1

        if "ExpectedInstructionCount" in items:
            performance_json["Instructions"][key]["ExpectedInstructionCount"] = items["ExpectedInstructionCount"]
        if "ExpectedArm64ASM" in items:
            performance_json["Instructions"][key]["ExpectedArm64ASM"] = items["ExpectedArm64ASM"]
        if "x86Insts" in performance_json["Instructions"][key]:
            d = performance_json["Instructions"][key]
            d.pop('x86InstructionCount', None)
            d = insert_before(d, "ExpectedInstructionCount",
                              ("x86InstructionCount", len(d["x86Insts"])))
            performance_json["Instructions"][key] = d

    # Output to the original file.
    with open(performance_json_path, "w") as json_file:
        json.dump(performance_json, json_file, indent=2)
        json_file.write("\n")

def main():
    if sys.version_info[0] < 3:
        logging.critical ("Python 3 or a more recent version is required.")

    if (len(sys.argv) < 3):
        logging.critical ("usage: %s <PerformanceTests.json> <NewNumbers.json>" % (sys.argv[0]))

    performance_json_path = sys.argv[1]
    new_json_numbers = sys.argv[2]

    try:
        with open(new_json_numbers) as json_file:
            new_json_numbers_text = json_file.read()
    except IOError:
        # If there isn't any new json numbers for this file, then it is safe to skip.
        return 0

    try:
        with open(performance_json_path) as json_file:
            performance_json_text = json_file.read()
    except IOError:
        logging.error("IOError!")
        return 1

    try:
        performance_json_data = json.loads(performance_json_text)
        if not isinstance(performance_json_data, dict):
            raise TypeError('JSON data must be a dict')

        new_json_numbers_data = json.loads(new_json_numbers_text)
        if not isinstance(new_json_numbers_data, dict):
            raise TypeError('JSON data must be a dict')

        return update_performance_numbers(performance_json_path, performance_json_data, new_json_numbers_data)
    except ValueError as ve:
        logging.error(f'JSON error: {ve}')
        return 1

    return 0

if __name__ == "__main__":
    # execute only if run as a script
    sys.exit(main())

