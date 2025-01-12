import simpy
import random
import statistics

NUM_NODES = 16
PROCESSORS_PER_NODE = 32
MEMORY_PER_NODE = 128  
SIMULATION_TIME = 10000

class Task:
    def __init__(self, task_id, processors, memory, priority, duration):
        self.task_id = task_id
        self.processors = processors
        self.memory = memory
        self.priority = priority
        self.duration = duration

class ComputeNode:
    def __init__(self, env, node_id, processors, memory):
        self.env = env
        self.node_id = node_id
        self.processors = simpy.Resource(env, capacity=processors)
        self.memory = memory
        self.current_memory = 0
        self.task_log = []
    def allocate_memory(self, amount):
        if self.current_memory + amount <= self.memory:
            self.current_memory += amount
            return True
        return False
    def release_memory(self, amount):
        self.current_memory -= amount
        if self.current_memory < 0:
            self.current_memory = 0

def scheduler(env, nodes, task_queue, stats):
    while True:
        if task_queue:
            task_queue.sort(key=lambda x: x.priority, reverse=True)
            task = task_queue.pop(0)
            suitable_nodes = [node for node in nodes if node.processors.capacity - len(node.processors.queue) >= task.processors and node.memory - node.current_memory >= task.memory]
            if suitable_nodes:
                node = min(suitable_nodes, key=lambda x: x.current_memory / x.memory)
                env.process(execute_task(env, node, task, stats, task_queue))
            else:
                task_queue.append(task)
                yield env.timeout(1)
        else:
            yield env.timeout(1)

def execute_task(env, node, task, stats, task_queue):
    with node.processors.request() as req:
        yield req        
        allocated = node.allocate_memory(task.memory)
        if not allocated:            
            node.processors.release(req)
            task_queue.append(task)
            return
        start_time = env.now
        node.task_log.append((task.task_id, start_time, start_time + task.duration))        
        stats['start_times'].append(start_time)
        stats['end_times'].append(start_time + task.duration)        
        yield env.timeout(task.duration)
        end_time = env.now
        node.release_memory(task.memory)        
        stats['makespan'] = max(stats['makespan'], end_time)        
        stats['node_load'][node.node_id].append(task.duration)
        print(f"Время {env.now}: Задача {task.task_id} завершена на узле {node.node_id}")

def generate_tasks(num_tasks):
    tasks = []
    for i in range(num_tasks):
        processors = random.randint(1, 8)
        memory = random.randint(1, 16)
        priority = random.randint(1, 10)
        duration = random.randint(100, 1000)
        tasks.append(Task(i, processors, memory, priority, duration))
    return tasks

def run_simulation():
    env = simpy.Environment()
    nodes = [ComputeNode(env, node_id=i, processors=PROCESSORS_PER_NODE, memory=MEMORY_PER_NODE) for i in range(NUM_NODES)]
    tasks = generate_tasks(1000)
    task_queue = tasks.copy()
    stats = {
        'start_times': [],
        'end_times': [],
        'makespan': 0,
        'node_load': {i: [] for i in range(NUM_NODES)}
    }
    env.process(scheduler(env, nodes, task_queue, stats))
    env.run(until=SIMULATION_TIME)
    print("\nРезультаты симуляции:")
    print(f"Makespan: {stats['makespan']}")
    avg_start = statistics.mean(stats['start_times']) if stats['start_times'] else 0
    avg_end = statistics.mean(stats['end_times']) if stats['end_times'] else 0
    print(f"Среднее время начала задач: {avg_start}")
    print(f"Среднее время окончания задач: {avg_end}")
    for node_id, loads in stats['node_load'].items():
        total_load = sum(loads)
        print(f"Узел {node_id} общая загрузка: {total_load}")
if __name__ == "__main__":
    run_simulation()
