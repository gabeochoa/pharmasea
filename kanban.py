

with open("todo.md") as file:
    lines = [line.rstrip() for line in file]

class Col:
    def __init__(self, line):
        self.name = line.split("##")[1].strip()
        self.items = []

    def add(self, line):
        if (self.name == "done" and len(self.items) >= 10):
            return

        # cutting '- [X] '
        value = line[6:]
        self.items.append(value)

columns = [];
active_column = None
for line in lines:
    if line.startswith("##"):
        columns.append(Col(line))
        active_column = columns[-1]
        continue
    # skip the **Complete**
    if line.startswith("**"):
        continue
    if line.startswith("-") and active_column != None:
        active_column.add(line)
        continue

## remove backlog


skip_col = ["backlog", "want for mvp"]

columns = filter(lambda x: x.name not in skip_col, columns)

#### now we just output the table

def pipe_join(items):
    return '|'.join(items)

def wrap_row(inside):
    return "|{}|\n".format(inside)

output_lines = []

# header
output_lines.append("\n")
output_lines.append(wrap_row(pipe_join([col.name for col in columns])))
output_lines.append(wrap_row(pipe_join([ ('-' * len(col.name)) for col in columns])))

max_row_idx = max([len(col.items) for col in columns])
row_idx = 0
cur_row = []

while(row_idx < max_row_idx):
    for col in columns:
        if(row_idx < len(col.items)):
            cur_row.append(col.items[row_idx])
        else:
            cur_row.append(" ")
    #
    output_lines.append((wrap_row(pipe_join(cur_row))))
    cur_row = []
    row_idx+=1

output_lines.append("\n")





##### WRITE TO FILES



index = 0
todo_index = 0
end_todo_index = 0
with open("README.md") as file:
    lines = [line for line in file]
    for line in lines:
        if(line.startswith("## TODOs")):
            todo_index = index
        if(line.startswith("## End TODO")):
            end_todo_index = index
        index += 1

lines = lines[0:todo_index+1] + output_lines + lines[end_todo_index:]

outF = open("README.md", "w")
outF.writelines(lines)
outF.close()
