import itertools

def calculate_LRU(accessPattern, accessPatternLen):
    
  ways = [' ', ' ']
  index = 0

  for i in range(accessPatternLen):
    
    # If no way has the address
    if (ways[0] != accessPattern[i] and 
        ways[1] != accessPattern[i] ):
  
      # Write address to the indexed way, and change the index
      ways[index] = accessPattern[i]
      index = (index + 1) % 2

    # If way[0] has the address
    elif (ways[0] == accessPattern[i] ):

      # Write address to way[0], and change index to 1
      ways[0] = accessPattern[i]
      index = 1

    # If way[1] has the address
    elif (ways[1] == accessPattern[i] ):

      # Write address to way[1], and change index to 0
      ways[1] = accessPattern[i]
      index = 0

    # print i , ways

  return ways

def calculate_RRIP(accessPattern, accessPatternLen):
    
  ways = [
    [' ', 0], 
    [' ', 0]]
  index = 0

  for i in range(accessPatternLen):
    
    # If no way has the address, and there is no lock
    if (ways[0][0] != accessPattern[i] and ways[0][1] == 0 and 
        ways[1][0] != accessPattern[i] and ways[1][1] == 0 ):
  
      # Write address to the indexed way, and change the index
      ways[index][0] = accessPattern[i]
      index = (index + 1) % 2

    # If no way has the address, but way[0] has lock
    elif (ways[0][0] != accessPattern[i] and ways[0][1] == 1 and 
          ways[1][0] != accessPattern[i] and ways[1][1] == 0 ):

      # Write address to way[1], change index to 0
      ways[1][0] = accessPattern[i]
      index = 0

    # If no way has the address, but way[1] has lock
    elif (ways[0][0] != accessPattern[i] and ways[0][1] == 0 and 
          ways[1][0] != accessPattern[i] and ways[1][1] == 1 ):

      # Write address to way[0], change index to 1
      ways[0][0] = accessPattern[i]
      index = 1

    # If way[0] has the address, but has no lock
    elif (ways[0][0] == accessPattern[i] and ways[0][1] == 0 ):

      # Write address to way[0], lock it, and change index to 1
      ways[0][0] = accessPattern[i]
      ways[0][1] = 1
      ways[1][1] = 0
      index = 1

    # If way[1] has the address, but has no lock
    elif (ways[1][0] == accessPattern[i] and ways[1][1] == 0 ):

      # Write address to way[1], lock it, and change index to 0
      ways[1][0] = accessPattern[i]
      ways[1][1] = 1
      ways[0][1] = 0
      index = 0

    # print i , ways

  return [ways[0][0], ways[1][0]]

def calculate_QUAD1(accessPattern, accessPatternLen):
    
  ways = [' ', ' ']
  ages = [None, None]
  
  INSERTION_AGE = 2 
  EVICTION_AGE = 3 

  for i in range(accessPatternLen):

    # If the access is cached, decrease its age
    if ways[0] == accessPattern[i]:
      ages[0] = max(0, ages[0]-1)
    elif ways[1] == accessPattern[i]:
      ages[1] = max(0, ages[1]-1)
    else:

        # If there is an empty way, introduce with age 2
        if ages[0] is None:
          ways[0] = accessPattern[i]
          ages[0] = INSERTION_AGE
        elif ages[1] is None:
          ways[1] = accessPattern[i]
          ages[1] = INSERTION_AGE
        else: # Look for eviction candidate
          found = False
          while not found:
            if (EVICTION_AGE not in ages):
              ages[0] += 1
              ages[1] += 1
            else:
              if ages[1] == EVICTION_AGE:
                ways[1] = accessPattern[i]
                ages[1] = INSERTION_AGE
              else:
                ways[0] = accessPattern[i]
                ages[0] = INSERTION_AGE
              found = True
  return ways

def calculate_QUAD2(accessPattern, accessPatternLen):
    
  lru_index  = 0
  ways = [' ', ' ']
  ages = [None, None]
  
  INSERTION_AGE = 2 
  EVICTION_AGE = 3 

  for i in range(accessPatternLen):

    # If the access is cached, decrease its age
    if ways[0] == accessPattern[i]:
      ages[0] = max(0, ages[0]-1)
      lru_index = 1
    elif ways[1] == accessPattern[i]:
      ages[1] = max(0, ages[1]-1)
      lru_index = 0
    else:

        # If there is an empty way, introduce with age 2
        if ages[0] is None:
          ways[0] = accessPattern[i]
          ages[0] = INSERTION_AGE
          lru_index = 1
        elif ages[1] is None:
          ways[1] = accessPattern[i]
          ages[1] = INSERTION_AGE
          lru_index = 0

        else: # Look for eviction candidate
          found = False
          while not found:
            if (EVICTION_AGE not in ages):
              ages[0] += 1
              ages[1] += 1
            else:
              if ages[0] == EVICTION_AGE and ages[1] == EVICTION_AGE:
                if lru_index == 0:
                  ways[0] = accessPattern[i]
                  ages[0] = INSERTION_AGE
                  lru_index = 1
                else:
                  ways[1] = accessPattern[i]
                  ages[1] = INSERTION_AGE
                  lru_index = 0
              elif ages[0] == EVICTION_AGE:
                ways[0] = accessPattern[i]
                ages[0] = INSERTION_AGE
                lru_index = 1
              else:
                ways[1] = accessPattern[i]
                ages[1] = INSERTION_AGE
                lru_index = 0
              found = True
  return ways

def calculate_QUAD3(accessPattern, accessPatternLen):
    
  ways = [' ', ' ']
  ages = [None, None]
  
  INSERTION_AGE = 1 
  EVICTION_AGE = 3 

  for i in range(accessPatternLen):

    # If the access is cached, set age to zero
    if ways[0] == accessPattern[i]:
      ages[0] = 0
    elif ways[1] == accessPattern[i]:
      ages[1] = 0
    else:

        # If there is an empty way, introduce with insertion age
        # look for empty way from RIGHT to LEFT
        if ages[1] is None:
          ways[1] = accessPattern[i]
          ages[1] = INSERTION_AGE
        elif ages[0] is None:
          ways[0] = accessPattern[i]
          ages[0] = INSERTION_AGE
        else: # Look for eviction candidate
          found = False
          while not found:
            if (EVICTION_AGE not in ages):
              ages[0] += 1
              ages[1] += 1
            else:
              if ages[1] == EVICTION_AGE:
                ways[1] = accessPattern[i]
                ages[1] = INSERTION_AGE
              else:
                ways[0] = accessPattern[i]
                ages[0] = INSERTION_AGE
              found = True
  return ways

def create_struct(accessPattern):
  patternLen = len(accessPattern)
  patternExtended = (accessPattern + (' ', ' ', ' ', ' ', ' ', ' '))[:6]

  # expectedLRU = accessPattern[-2:]
  expectedLRU = calculate_LRU(accessPattern, patternLen)
  expectedRRIP = calculate_RRIP(accessPattern, patternLen)
  expectedQUAD1 = calculate_QUAD1(accessPattern, patternLen)
  expectedQUAD2 = calculate_QUAD2(accessPattern, patternLen)
  expectedQUAD3 = calculate_QUAD3(accessPattern, patternLen)

  return [patternExtended, patternLen, expectedLRU, expectedRRIP, expectedQUAD1, expectedQUAD2, expectedQUAD3]

def convert_C_struct(struct):
  
  print("#define POLICY_TEST_COUNT " + str(len(struct)))

  print("struct policyTestInfo pti[" + str(len(struct)) + "] = {")
  for item in struct:
    print("  { {"),
    print("'" + "','".join(item[0])+"' },"),
    print(str(item[1])+", {"),
    print("'" + "','".join(item[2])+"' }, {"),
    print("'" + "','".join(item[3])+"' }, {"),
    print("'" + "','".join(item[4])+"' }, {"),
    print("'" + "','".join(item[5])+"' }, {"),
    print("'" + "','".join(item[6])+"' } },")
  print("};");
  
# Get all combinations with 3, 4 and 5 repetitions
pattern  = list(itertools.product(['A', 'B', 'C'], repeat=3))
pattern += list(itertools.product(['A', 'B', 'C'], repeat=4))
pattern += list(itertools.product(['A', 'B', 'C'], repeat=5))

# Filter out all combinations wheere all 3 elements do not exist
pattern = [x for x in pattern if len(set(x))==3]

struct = [create_struct(x) for x in pattern]

convert_C_struct(struct)
