# check my logs against nintendulator CPU register debug log


def get_my_log(ind):
    log_line = my_log[ind]
    pc = log_line[3:7]
    a = log_line[10:12]
    x = log_line[15:17]
    y = log_line[20:22]
    p = log_line[25:27]
    sp = log_line[31:33]
    cyc = log_line[38:41]
    sc = log_line[45:48]
    return {'pc':pc,
            'a':a,
            'x':x,
            'y':y,
            'p':p,
            'sp':sp,
            'cyc': int(cyc),
            'sc': int(sc)}



def get_right_log(ind):
    log_line = correct_log[ind]
    pc = log_line[:4]
    a = log_line[50:52]
    x = log_line[55:57]
    y = log_line[60:62]
    p = log_line[65:67]
    sp = log_line[71:73]
    cyc = log_line[78:81]
    sc = int(log_line[85:88])
    if sc == -1:
        sc = 261
    return {'pc':pc,
            'a':a,
            'x':x,
            'y':y,
            'p':p,
            'sp':sp,
            'cyc': int(cyc),
            'sc': sc}




with open("dk.debug", "r") as f:
    correct_log = f.readlines()

with open("dk.log", "r") as f:
    my_log = f.readlines()


with open("index.txt", "r") as f:
    hi = f.readlines()[0]
    hi = hi.replace("(", " ")
    hi = hi.replace(")", " ")
    hi = hi.replace(",", "")
    hi = hi.replace("  ", " ")
    hi = hi.split()
    hi = list(map(int, hi))
    m = max(hi)
    print(m)
    print(hi[:500])



diff = 0

for i in range(0, len(correct_log)):
    if i >= 42595:
        if get_my_log(i)['pc'] != get_right_log(i + 1)['pc']:
            print(i)
            print(get_my_log(i))
            print(get_right_log(i + 1))
            break
        continue
        
    if get_my_log(i)['cyc'] != get_right_log(i)['cyc']:
        print(i)
        print(get_my_log(i))
        print(get_right_log(i))
        break


