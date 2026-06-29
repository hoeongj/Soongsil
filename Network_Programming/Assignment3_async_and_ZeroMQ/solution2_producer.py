import zmq
import json
import time
import random
from pymemcache.client import base as memcache_client

# ZeroMQ endpoints
PUB_ADDR  = 'tcp://127.0.0.1:5555'   # Publish tasks
PULL_ADDR = 'tcp://127.0.0.1:5556'   # Receive answers

# Task generation settings
OPERATORS     = ['+', '-', '*']
OPERAND_RANGE = (1, 50)
TASK_INTERVAL = 3   # seconds between tasks


# TODO: Implement the task producer.
#
# - Publish math tasks periodically via ZeroMQ PUB.
# - Collect and verify worker answers via ZeroMQ PULL.
# - Track per-worker scores in Memcached.
# - Must interleave publishing and receiving without
#   blocking on either.
#
# Hint: Reference Chapter 8 for ZeroMQ and Memcached
#       patterns.


import sys

# Extra import and stdout setting are below the template so the given
# starter code remains unchanged.
sys.stdout.reconfigure(encoding='utf-8')


def solve_math(operand1, operator, operand2):
    # This block mirrors the same arithmetic logic used by the workers.
    # The producer uses it to know the correct answer for grading.
    # Calculate the correct answer for the created task.
    if operator == '+':
        return operand1 + operand2
    if operator == '-':
        return operand1 - operand2
    if operator == '*':
        return operand1 * operand2
    raise ValueError('Unsupported operator: ' + operator)


def make_task(task_id):
    # This block creates the task object that will be published to workers.
    # It returns both the public task and the private correct answer.
    # Create one random math problem in the required JSON format.
    # The answer is kept locally because workers only receive the task data.
    operand1 = random.randint(OPERAND_RANGE[0], OPERAND_RANGE[1])
    operand2 = random.randint(OPERAND_RANGE[0], OPERAND_RANGE[1])
    operator = random.choice(OPERATORS)

    # This dictionary follows the exact Task JSON format in the assignment.
    task = {
        'task_id': task_id,
        'operand1': operand1,
        'operator': operator,
        'operand2': operand2,
    }
    return task, solve_math(operand1, operator, operand2)


def read_score(cache, worker_id):
    # This block reads a worker's current score from Memcached.
    # Returning zero on errors keeps the producer running.
    # Scores are stored in Memcached with key format score:<worker_id>.
    key = 'score:' + worker_id
    try:
        value = cache.get(key)
    except Exception as e:
        print('Memcached read error for ' + key + ': ' + str(e), flush=True)
        return 0

    if value is None:
        # No key in Memcached means the worker has not scored yet.
        return 0

    try:
        # pymemcache returns bytes, but int() can convert byte strings.
        return int(value)
    except (TypeError, ValueError):
        # If a stored value is broken, use zero instead of crashing.
        return 0


def write_score(cache, worker_id, score):
    # This block writes the updated score after a correct answer.
    # Store the updated cumulative score back to Memcached.
    key = 'score:' + worker_id
    try:
        cache.set(key, str(score))
    except Exception as e:
        print('Memcached write error for ' + key + ': ' + str(e), flush=True)


def handle_answer(cache, expected_answers, scored_answers, message):
    # This block processes one JSON answer received from the PULL socket.
    # It validates fields, checks correctness, and updates Memcached.
    # Validate one worker answer, check it, and update the scoreboard.
    # Bad messages are ignored instead of stopping the producer.
    task_id = message.get('task_id')
    worker_id = message.get('worker_id')
    answer = message.get('answer')

    if task_id is None or worker_id is None or answer is None:
        # Missing required fields means the answer cannot be graded.
        print('Invalid answer message: ' + str(message), flush=True)
        return

    # Look up the correct answer that was saved when the task was published.
    expected = expected_answers.get(task_id)
    if expected is None:
        # This can happen if an old or malformed task_id is submitted.
        print('[Task ' + str(task_id) + '] ' + worker_id
              + ' answered ' + str(answer) + ' — Unknown task!', flush=True)
        return

    score_key = (task_id, worker_id)
    if score_key in scored_answers:
        # Do not count the same worker twice for one task.
        print('[Task ' + str(task_id) + '] ' + worker_id
              + ' answered ' + str(answer) + ' — Duplicate!', flush=True)
        return

    # From here, this worker's answer for this task has been handled once.
    scored_answers.add(score_key)
    if answer == expected:
        # Correct answers increase the cumulative score by one.
        score = read_score(cache, worker_id) + 1
        write_score(cache, worker_id, score)
        print('[Task ' + str(task_id) + '] ' + worker_id
              + ' answered ' + str(answer) + ' — Correct!', flush=True)
    else:
        print('[Task ' + str(task_id) + '] ' + worker_id
              + ' answered ' + str(answer) + ' — Incorrect!', flush=True)


def main():
    # Setup block:
    # Create ZeroMQ sockets and the Memcached client before the main loop.
    # PUB sends tasks to every subscribed worker.
    # PULL collects answers from all workers.
    context = zmq.Context()
    pub_socket = context.socket(zmq.PUB)
    pull_socket = context.socket(zmq.PULL)
    cache = memcache_client.Client(('127.0.0.1', 11211))

    # State block:
    # expected_answers stores correct answers by task_id.
    # scored_answers prevents double scoring by the same worker.
    # expected_answers is used to grade later worker submissions.
    expected_answers = {}
    scored_answers = set()
    task_id = 1
    next_task_time = time.monotonic()

    # Poller lets the producer check for answers without blocking forever.
    poller = zmq.Poller()

    try:
        # Bind block:
        # The producer owns both endpoints because workers connect to it.
        pub_socket.bind(PUB_ADDR)
        pull_socket.bind(PULL_ADDR)
        poller.register(pull_socket, zmq.POLLIN)

        print('Producer started.', flush=True)
        while True:
            # Main loop block:
            # One loop handles both scheduled publishing and answer polling.
            now = time.monotonic()
            if now >= next_task_time:
                # Publishing block:
                # Make a task, remember its answer, and send JSON by PUB.
                # Publish a new task when the interval has passed.
                # time.monotonic() is used so clock changes do not affect it.
                task, answer = make_task(task_id)
                expected_answers[task_id] = answer

                pub_socket.send_string(json.dumps(task))
                print('[Task ' + str(task_id) + '] Published: '
                      + str(task['operand1']) + ' ' + task['operator'] + ' '
                      + str(task['operand2']), flush=True)

                task_id += 1
                next_task_time = now + TASK_INTERVAL

            # Poll briefly so task publishing and answer receiving interleave.
            events = dict(poller.poll(100))
            if pull_socket in events:
                # Receiving block:
                # There may be multiple answers waiting, so drain the socket.
                while True:
                    try:
                        # Drain all answers that are already waiting.
                        # NOBLOCK prevents the producer from getting stuck here.
                        raw_message = pull_socket.recv_string(flags=zmq.NOBLOCK)
                    except zmq.Again:
                        break

                    try:
                        # JSON parsing is separated so malformed data is skipped.
                        message = json.loads(raw_message)
                    except json.JSONDecodeError:
                        print('Invalid JSON answer: ' + raw_message,
                              flush=True)
                        continue

                    handle_answer(cache, expected_answers, scored_answers,
                                  message)

    except KeyboardInterrupt:
        # Ctrl+C is used for normal shutdown in terminal testing.
        print('\nProducer stopped.', flush=True)
    finally:
        # Cleanup block:
        # Close external resources so ports are released immediately.
        pub_socket.close(0)
        pull_socket.close(0)
        context.term()

        try:
            cache.close()
        except Exception:
            pass


if __name__ == '__main__':
    main()
