
from collections import defaultdict

includes = defaultdict(list)
children = defaultdict(list)

def get_top_10():
    print( "====================" +
            " most directly included" +
            "====================")
    # files included the most
    with_count = []
    for (k,v) in children.items():
        with_count.append( (k,len(v)) )
    with_count.sort(key=lambda x: -x[1])
    for tup in with_count[:20]:
        print(tup)
    print( "====================" +
            " end most included " +
            "====================")

def fetch_all_headers(file, headers):
    processed = []
    to_process = headers[::]

    while( len(to_process) > 0):
        head =  to_process.pop();
        processed.append(head)

        if head not in includes:
            continue
        new_files = [header for header in includes[head] if header not in processed]
        to_process += new_files

    return processed

def biggest_includers():
    print( "====================" +
            " biggest includers " +
            "====================")
    # files that include the most headers
    # (recursively)
    recur = []
    for (file,headers) in includes.items():
        folded = fetch_all_headers(file, list(headers))
        val = (file, len(folded))
        recur.append(val)

    recur.sort(key=lambda x: -x[1])
    for tup in recur[:20]:
        print(tup)

    print( "====================" +
            " end biggest includers " +
            "====================")

if __name__ == "__main__":
    input_file = "output/source.dot"

    with open(input_file, 'r') as f:
        content = f.read()

    for line in content.split("\n"):
        if '->' not in line:
            continue
        a = [s.strip().replace('"', '') for s in line.split("->")];
        children[a[1]].append(a[0])
        includes[a[0]].append(a[1])

    get_top_10();
    biggest_includers()


