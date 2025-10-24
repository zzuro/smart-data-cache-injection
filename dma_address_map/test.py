def find_self_referencing_rmp(start, end):
    KB = lambda x: x * 1024
    for x in range(start, end, KB(4)):
        page_aligned = (start + 0x4000 + (x >> 8)) & ~(KB(4) - 1)
        if page_aligned == x:
            return x
    return -1


res = find_self_referencing_rmp(0x0000000091900000, 0x0000000099efffff)
if res != -1:
    print(f"Self-referencing RMP entry found at physical address: 0x{res:x}")
else:
    print("Self-referencing RMP entry not found in the given range.")