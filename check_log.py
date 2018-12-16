# check my logs against nintendulator CPU register debug log


def get_my_log(ind):
    log_line = my_log[ind]
    pc = log_line[3:7]
    a = log_line[10:12]
    x = log_line[15:17]
    y = log_line[20:22]
    p = log_line[25:27]
    sp = log_line[31:33]
    return {'pc':pc, 'a':a, 'x':x, 'y':y, 'p':p, 'sp':sp}



def get_right_log(ind):
    log_line = correct_log[ind]
    pc = log_line[:4]
    a = log_line[50:52]
    x = log_line[55:57]
    y = log_line[60:62]
    p = log_line[65:67]
    # mask out the B flag, leave in same format
    p = str(hex(int(p, 16) & 0b11001111))[2:].zfill(2)
    sp = log_line[71:73]
    return {'pc':pc, 'a':a, 'x':x, 'y':y, 'p':p, 'sp':sp}




with open("nestest.log", "r") as f:
    correct_log = f.readlines()

with open("my_nestest.log", "r") as f:
    my_log = f.readlines()



for i in range(0, len(my_log)):
    if get_my_log(i)['pc'] != get_right_log(i)['pc']:
        print(i)
        print(get_my_log(i))
        print(get_right_log(i))
        break


