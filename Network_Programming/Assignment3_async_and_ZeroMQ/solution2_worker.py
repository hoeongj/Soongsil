import zmq
import json
import sys
from pymemcache.client import base as memcache_client

# ZeroMQ endpoints — must match producer
PUB_ADDR  = 'tcp://127.0.0.1:5555'   # Subscribe to tasks
PUSH_ADDR = 'tcp://127.0.0.1:5556'   # Send answers


# TODO: Implement the task worker.
#
# Usage: python solution2_worker.py <worker_id>
#
# - Subscribe to tasks from the producer via ZeroMQ SUB.
# - Compute answers and send them back via ZeroMQ PUSH.
# - Query own score from Memcached after each submission.
#
# Hint: Reference Chapter 8 for ZeroMQ and Memcached
#       patterns.


import time

# Extra import and stdout setting are below the template so the given
# starter code remains unchanged.
sys.stdout.reconfigure(encoding='utf-8')


def solve_task(task):
    # This block computes the answer for one received task.
    # It assumes the JSON fields were provided by the producer.
    # Calculate the answer from the received JSON task dictionary.
    # The producer and worker use the same operator rules.
    operand1 = task['operand1']
    operand2 = task['operand2']
    operator = task['operator']

    if operator == '+':
        return operand1 + operand2
    if operator == '-':
        return operand1 - operand2
    if operator == '*':
        return operand1 * operand2
    raise ValueError('Unsupported operator: ' + operator)


def read_score(cache, worker_id):
    # This block reads the current score after a worker sends an answer.
    # If Memcached is not ready, the worker prints zero instead of crashing.
    # The worker only reads its own score from Memcached.
    key = 'score:' + worker_id
    try:
        value = cache.get(key)
    except Exception as e:
        print('Memcached read error for ' + key + ': ' + str(e), flush=True)
        return 0

    if value is None:
        # Missing score means this worker has zero points so far.
        return 0

    try:
        # pymemcache returns bytes, but int() can convert byte strings.
        return int(value)
    except (TypeError, ValueError):
        # A corrupted score should not stop the worker process.
        return 0


def wait_for_score(cache, worker_id, old_score):
    # This block handles the small timing gap between PUSH send and score update.
    # The producer updates Memcached after receiving the answer.
    # Give the producer a short time to update Memcached after the answer.
    deadline = time.monotonic() + 1.0
    score = old_score

    while time.monotonic() < deadline:
        score = read_score(cache, worker_id)
        if score > old_score:
            # Stop early when the producer has written the new score.
            break
        time.sleep(0.05)

    return score


def main():
    # Argument block:
    # The worker ID is required for the answer JSON and score key.
    # The worker id is required so the producer can keep separate scores.
    if len(sys.argv) != 2:
        print('Usage: python solution2_worker.py <worker_id>', flush=True)
        return

    worker_id = sys.argv[1]
    # Setup block:
    # Create sockets and the Memcached client used by this worker.
    # SUB receives published tasks and PUSH sends answers to the producer.
    context = zmq.Context()
    sub_socket = context.socket(zmq.SUB)
    push_socket = context.socket(zmq.PUSH)
    cache = memcache_client.Client(('127.0.0.1', 11211))

    try:
        # Connection block:
        # Workers connect because the producer binds both endpoints.
        sub_socket.connect(PUB_ADDR)
        # Empty subscription means this worker accepts every task topic.
        sub_socket.setsockopt_string(zmq.SUBSCRIBE, '')
        push_socket.connect(PUSH_ADDR)

        print('Worker ' + worker_id + ' started.', flush=True)
        while True:
            # Main worker loop:
            # Receive one task, compute it, send the answer, then show score.
            # Wait for the next task from the producer.
            # This blocking receive is okay because the worker only has one job.
            raw_message = sub_socket.recv_string()

            try:
                # Parsing block:
                # Convert JSON text to a dictionary and verify required fields.
                task = json.loads(raw_message)
                answer = solve_task(task)
                task_id = task['task_id']
                operand1 = task['operand1']
                operator = task['operator']
                operand2 = task['operand2']
            except (json.JSONDecodeError, KeyError, ValueError) as e:
                print('Invalid task: ' + str(e), flush=True)
                continue

            # Read the old score before sending, then wait for a newer score.
            # This makes the printed score match the answer just submitted.
            old_score = read_score(cache, worker_id)
            answer_message = {
                'task_id': task_id,
                'worker_id': worker_id,
                'answer': answer,
            }

            # Sending block:
            # PUSH sends the answer to the producer's PULL socket.
            push_socket.send_string(json.dumps(answer_message))
            print('[Task ' + str(task_id) + '] Received: ' + str(operand1)
                  + ' ' + operator + ' ' + str(operand2)
                  + ' → Sending answer: ' + str(answer), flush=True)

            score = wait_for_score(cache, worker_id, old_score)
            print('My score: ' + str(score), flush=True)

    except KeyboardInterrupt:
        # Ctrl+C is used for normal shutdown in terminal testing.
        print('\nWorker stopped.', flush=True)
    finally:
        # Cleanup block:
        # Close sockets and Memcached client before the process exits.
        sub_socket.close(0)
        push_socket.close(0)
        context.term()

        try:
            cache.close()
        except Exception:
            pass


if __name__ == '__main__':
    main()
