#!/usr/bin/python
# Modified from http://www.diva-portal.org/smash/get/diva2:1116701/FULLTEXT01.pdf

import sys, math, random, copy, string

neighbors_of = {}
#                     nw   ne     e    se   sw   w
neighbors_of['q'] = [            'w', 'a']
neighbors_of['w'] = [            'e', 's', 'a', 'q']
neighbors_of['e'] = [            'r', 'd', 's', 'w']
neighbors_of['r'] = [            't', 'f', 'd', 'e']
neighbors_of['t'] = [            'z', 'g', 'f', 'r']
neighbors_of['z'] = [            'u', 'h', 'g', 't']
neighbors_of['u'] = [            'i', 'j', 'h', 'z']
neighbors_of['i'] = [            'o', 'k', 'j', 'u']
neighbors_of['o'] = [            'p', 'l', 'k', 'i']
neighbors_of['p'] = [                      'l', 'o']

neighbors_of['a'] = ['q', 'w', 's', 'y']
neighbors_of['s'] = ['w', 'e', 'd', 'x', 'y', 'a']
neighbors_of['d'] = ['e', 'r', 'f', 'c', 'x', 's']
neighbors_of['f'] = ['r', 't', 'g', 'v', 'c', 'd']
neighbors_of['g'] = ['t', 'z', 'h', 'b', 'v', 'f']
neighbors_of['h'] = ['z', 'u', 'j', 'n', 'b', 'g']
neighbors_of['j'] = ['u', 'i', 'k', 'm', 'n', 'h']
neighbors_of['k'] = ['i', 'o', 'l',      'm', 'j']
neighbors_of['l'] = ['o', 'p',                'k']

neighbors_of['y'] = ['a', 's', 'x']
neighbors_of['x'] = ['s', 'd', 'c',            'y']
neighbors_of['c'] = ['d', 'f', 'v',            'x']
neighbors_of['v'] = ['f', 'g', 'b',            'c']
neighbors_of['b'] = ['g', 'h', 'n',            'v']
neighbors_of['n'] = ['h', 'j', 'm',            'b']
neighbors_of['m'] = ['j', 'k',                 'n']

keys = sorted(neighbors_of.keys())
dists = {el:{} for el in keys}

def distance(start, end, raw):
    if start == end:
        if raw:
            return 0
        else:
            return 1

    visited = [start]
    queue = []

    for key in neighbors_of[start]:
        queue.append({'char': key, 'dist': 1})

    while True:
        key = queue.pop(0)
        visited.append(key['char'])
        if key['char'] == end:
            return key['dist']

        for neighbor in neighbors_of[key['char']]:
            if neighbor not in visited:
                        queue.append({'char': neighbor, 'dist':
key['dist']+1})

def alldists(type, verbose):
    if type == "linear":
        longest_dist = 0
        avgdist = 0
        for i in range(len(keys)):
            for j in range(len(keys)):
                dist = distance(keys[i], keys[j], False)
                dists[keys[i]][keys[j]] = 2 - (2 * dist / 9.0)
                avgdist += dists[keys[i]][keys[j]]
                if dist > longest_dist:
                    longest_dist = dist
        key_dist = longest_dist
        avgdist /= len(keys) ** 2 + 0.0
        if verbose:
            print("Average distance: " + str(avgdist))

        avgdisttwo = 0

        for i in range(len(keys)):
            for j in range(len(keys)):
                dists[keys[i]][keys[j]] /= avgdist
                avgdisttwo += dists[keys[i]][keys[j]]

        avgdisttwo /= len(keys) ** 2 + 0.0
        if verbose:
            print("Average distance after normalizing: " +str(avgdisttwo))
            print("Longest distance: " + str(key_dist))
            print("Longest logarithmed: " + str(math.log(key_dist)))
            print("Logarithmed & normalized: " + str(math.log(key_dist) / math.log(9)))
            print(str(dists).replace("'", '"'))

    elif type == "neighbors":
        longest_dist = 0
        avgdist = 0
        for i in range(len(keys)):
            for j in range(len(keys)):
                dist = distance(keys[i], keys[j], False)
                if dist == 1:
                    dists[keys[i]][keys[j]] = 2.0
                else:
                    dists[keys[i]][keys[j]] = 1.0

                avgdist += dists[keys[i]][keys[j]]
                if dist > longest_dist:
                    longest_dist = dist
        key_dist = longest_dist
        avgdist /= len(keys) ** 2 + 0.0
        if verbose:
            print("Average distance: " + str(avgdist))

        avgdisttwo = 0

        for i in range(len(keys)):
            for j in range(len(keys)):
                dists[keys[i]][keys[j]] /= avgdist
                avgdisttwo += dists[keys[i]][keys[j]]

        avgdisttwo /= len(keys) ** 2 + 0.0
        if verbose:
            print("Average distance after normalizing: " + str(avgdisttwo))
            print("Longest distance: " + str(key_dist))
            print("Longest logarithmed: " + str(math.log(key_dist)))
            print("Logarithmed & normalized: " + str(math.log(key_dist) / math.log(9)))
            print(str(dists).replace("'", '"'))

    elif type == "raw":
        longest_dist = 0
        avgdist = 0
        for i in range(len(keys)):
            for j in range(len(keys)):
                dists[keys[i]][keys[j]] = distance(keys[i], keys[j], True)
                avgdist += dists[keys[i]][keys[j]]
                if dists[keys[i]][keys[j]] > longest_dist:
                    longest_dist = dists[keys[i]][keys[j]]
        key_dist = longest_dist
        avgdist /= len(keys) ** 2 + 0.0

        buckets = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

        for i in range(len(keys)):
            for j in range(len(keys)):
                buckets[dists[keys[i]][keys[j]]] += 1

        if verbose:
            print("Average distance: " + str(avgdist))
            print("Longest distance: " + str(key_dist))
            print("Buckets: " + str(buckets))
            print(str(dists).replace("'", '"'))
    return copy.deepcopy(dists)

def main():
    if len(sys.argv) == 2:
        if sys.argv[1] == "gen":
            for letter1 in string.ascii_lowercase:
                for letter2 in string.ascii_lowercase:
                    print(f"{letter1} {letter2} {distance(letter1, letter2, True)}")
        else:
            alldists(sys.argv[1], True)
    else:
        key_dist = distance(sys.argv[1], sys.argv[2], True)
        print(str(key_dist))

if __name__ == "__main__":
    main()
