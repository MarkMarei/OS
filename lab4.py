import redis
import time
import uuid
import multiprocessing

client_processes_waiting = [0, 1, 1, 1, 4]

class Redlock:
    def __init__(self, redis_nodes):
        """
        Initialize Redlock with a list of Redis node addresses.
        :param redis_nodes: List of (host, port) tuples.
        """
        self.redis_clients = [redis.StrictRedis(host=host, port=port, decode_responses=True) for host, port in redis_nodes]
        
        self.majority = (len(self.redis_clients) // 2) + 1
        pass

    def acquire_lock(self, resource, ttl):
        """
        Try to acquire a distributed lock.
        :param resource: The name of the resource to lock.
        :param ttl: Time-to-live for the lock in milliseconds.
        :return: Tuple (lock_acquired, lock_id).
        """
        lock_id = str(uuid.uuid4())
        start = time.time()
        
        aquired = 0
        for client in self.redis_clients:
            try:
                if client.set(resource, lock_id, nx=True, px=ttl):
                    aquired += 1
            except Exception as e:
                pass
            
        
        time_taken = (time.time() - start) * 1000
        if (aquired >= self.majority and time_taken <= ttl):
            return True, lock_id
        
        self.release_lock(resource, lock_id)
        return False, None

    def release_lock(self, resource, lock_id):
        """
        Release the distributed lock.
        :param resource: The name of the resource to unlock.
        :param lock_id: The unique lock ID to verify ownership.
        """
        for client in self.redis_clients:
            try:
                if client.get(resource) == lock_id:
                    client.delete(resource)            
            except Exception as e:
                pass

def client_process(redis_nodes, resource, ttl, client_id):
    """
    Function to simulate a single client process trying to acquire and release a lock.
    """
    time.sleep(client_processes_waiting[client_id])

    redlock = Redlock(redis_nodes)
    print(f"\nClient-{client_id}: Attempting to acquire lock...")
    lock_acquired, lock_id = redlock.acquire_lock(resource, ttl)

    if lock_acquired:
        print(f"\nClient-{client_id}: Lock acquired! Lock ID: {lock_id}")
        # Simulate critical section
        time.sleep(3)  # Simulate some work
        redlock.release_lock(resource, lock_id)
        print(f"\nClient-{client_id}: Lock released!")
    else:
        print(f"\nClient-{client_id}: Failed to acquire lock.")

if __name__ == "__main__":
    # Define Redis node addresses (host, port)
    redis_nodes = [
        ("localhost", 63791),
        ("localhost", 63792),
        ("localhost", 63793),
        ("localhost", 63794),
        ("localhost", 63795),
    ]

    resource = "shared_resource"
    ttl = 5000  # Lock TTL in milliseconds (5 seconds)

    # Number of client processes
    num_clients = 5

    # Start multiple client processes
    processes = []
    for i in range(num_clients):
        process = multiprocessing.Process(target=client_process, args=(redis_nodes, resource, ttl, i))
        processes.append(process)
        process.start()

    for process in processes:
        process.join()
