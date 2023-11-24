import math
import random

class Connection:
    def __init__(self, target:int, min:float, max:float, strength:float):
        self.target   = target
        self.min      = min
        self.max      = max
        self.strength = strength

class Neuron:
    def __init__(self, x:float, y:float, connectTicks:int):
        self.x = x
        self.y = y

        self.connections:list[Connection] = []

        self.value       = 0.0
        self.maxStrength = 0.0

        self.connectTick  = 0
        self.connectTicks = connectTicks
    
class Brain:
    def __init__(self):
        self.neurons:list[Neuron] = []

    def populate(self, n:int):
        for i in range(n):
            for j in range(n):
                self.neurons.append(Neuron((i - n/2) * 3, (j - n/2) * 3, random.randint(5, 25)))

    def findNearestNeurons(self, neuron:Neuron, maxDistance:float = 10.0):
        neighbours:list[tuple[int,Neuron]] = []
        for i, n in enumerate(self.neurons):
            if n == neuron:
                continue

            dx = n.x - neuron.x
            dy = n.y - neuron.y
            distance = math.sqrt(dx * dx + dy * dy)
            if distance <= maxDistance:
                neighbours.append((i, n))
        return neighbours
    
    def tick(self):
        newConnections = 0

        nextValues = [0.0 for i in range(len(self.neurons))]
        for i, n in enumerate(self.neurons):
            for con in n.connections:
                if n.value >= con.min and n.value <= con.max:
                    nextValues[con.target] += con.strength
        for i, n in enumerate(self.neurons):
            n.value = nextValues[i] / max(n.maxStrength, 1.0)
            n.connectTick += 1
            if n.connectTick >= n.connectTicks:
                n.connectTick = 0
                neighbours = self.findNearestNeurons(n)
                bestNeighbour:tuple[int,Neuron] = None
                closestDist:float = 20.0
                for neighbour in neighbours:
                    good = True
                    for con in n.connections:
                        if neighbour[0] == con.target:
                            good = False
                            break
                    if not good:
                        continue

                    dx = neighbour[1].x - n.x
                    dy = neighbour[1].y - n.y
                    dist = math.sqrt(dx * dx + dy * dy)
                    if dist < closestDist:
                        closestDist   = dist
                        bestNeighbour = neighbour

                if bestNeighbour == None:
                    continue

                a = random.random()
                b = random.random()
                s = random.random()
                n.connections.append(Connection(bestNeighbour[0], min(a, b), max(a, b), s))
                n.maxStrength += s
                a = random.random()
                b = random.random()
                s = random.random()
                bestNeighbour[1].connections.append(Connection(i, min(a, b), max(a, b), s))
                bestNeighbour[1].maxStrength += s
                newConnections += 2

        return newConnections
    
def main():
    brain = Brain()
    brain.populate(32)

    inputI  = random.randint(0, len(brain.neurons))
    outputI = random.randint(0, len(brain.neurons))
    for i in range(5 * 10):
        for n in brain.neurons:
            n.value = random.random()
        newConnections = brain.tick()
        print(f"{brain.neurons[outputI].value} | {newConnections}")
    print("Kickstart over")
    for i in range(1024):
        # brain.neurons[inputI].value = random.random()
        newConnections = brain.tick()
        print(f"{brain.neurons[outputI].value} | {newConnections}")

main()