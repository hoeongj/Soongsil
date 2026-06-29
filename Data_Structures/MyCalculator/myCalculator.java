//20221528_홍성주
import java.util.Scanner;

public class myCalculator {

    static class Stack {
        private String[] stackArray;
        private int top;

        public Stack(int size) {
            stackArray = new String[size];
            top = -1;
        }

        public void push(String s) {
            stackArray[++top] = s;
        }

        public String pop() {
            return stackArray[top--];
        }

        public String peek() {
            return stackArray[top];
        }

        public boolean isEmpty() {
            return top == -1;
        }
    }

    private static boolean isOper(String oper) {
        return oper.equals("+") || oper.equals("-") || oper.equals("*") || oper.equals("/");
    }

    private static int operRank(String oper2) {
        if (oper2.equals("+") || oper2.equals("-")) return 1;
        else if (oper2.equals("*") || oper2.equals("/")) return 2;
        else return -1;
    }

    public static String infixToPostfix(String infix) {
        StringBuilder postfix = new StringBuilder();

        infix = infix.replace("(", " ( ").replace(")", " ) ");

        String[] tokens = infix.trim().split("\\s+");
        Stack stack = new Stack(tokens.length);

        try {
            for (String token : tokens) {
                if (isNumeric(token)) postfix.append(token).append(" ");
                else if (token.equals("(")) {
                    stack.push(token);
                } else if (token.equals(")")) {
                    while (!stack.isEmpty() && !stack.peek().equals("(")) {
                        postfix.append(stack.pop()).append(" ");
                    }
                    if (!stack.isEmpty() && stack.peek().equals("(")) {
                        stack.pop();
                    } else throw new IllegalArgumentException("잘못된 입력!");
                } else if (isOper(token)) {
                    while (!stack.isEmpty() && operRank(stack.peek()) >= operRank(token)) {
                        postfix.append(stack.pop()).append(" ");
                    }
                    stack.push(token);
                } else throw new IllegalArgumentException("잘못된 입력!");
            }

            while (!stack.isEmpty()) {
                if (stack.peek().equals("(")) throw new IllegalArgumentException("잘못된 입력!");
                postfix.append(stack.pop()).append(" ");
            }

        } catch (Exception e) {
            throw new IllegalArgumentException(e.getMessage());
        }
        return postfix.toString().trim();
    }


    public static double calcPostfix(String postfix) {
        String[] tokens = postfix.split("\\s+");
        double[] stack = new double[tokens.length];
        int top = -1;

        try {
            for (String token : tokens) {
                if (isNumeric(token)) stack[++top] = Double.parseDouble(token);
                else if (isOper(token)) {
                    if (top < 1) throw new IllegalArgumentException("잘못된 입력");
                    double operand2 = stack[top--];
                    double operand1 = stack[top--];
                    double result = 0;

                    switch (token) {
                        case "+":
                            result = operand1 + operand2;
                            break;
                        case "-":
                            result = operand1 - operand2;
                            break;
                        case "*":
                            result = operand1 * operand2;
                            break;
                        case "/":
                            result = operand1 / operand2;
                            break;
                        default:
                            throw new IllegalArgumentException("지원하지 않는 연산자입니다.");
                    }

                    stack[++top] = result;
                } else throw new IllegalArgumentException("잘못된 입력!");
            }

            if (top != 0) throw new IllegalArgumentException("잘못된 입력!");

        } catch (Exception e) {
            throw new IllegalArgumentException("잘못된 입력!");
        }
        return stack[top];
    }

    private static boolean isNumeric(String str) {
        try {
            Double.parseDouble(str);
            return true;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    public static void main(String[] args) {
        Scanner sc = new Scanner(System.in);
        System.out.println("======= MyCalculator ========");
        System.out.println("MyCalculator를 사용을 환영합니다.");

        boolean still = true;

        while (still) {
            System.out.print("\nInfix로 수식을 입력하시오.");
            sc.nextLine();
            System.out.print(">");
            String infix = sc.nextLine();

            String postfix;
            try {
                postfix = infixToPostfix(infix);
                System.out.println(">Postfix로 변환 : " + postfix);
            } catch (IllegalArgumentException e) {
                System.out.println("잘못된 입력!");
                continue;
            }

            while (true) {
                System.out.print("\n계산을 시작할까요? (Y/N)\n>");
                String go = sc.nextLine();

                if (go.equals("y") || go.equals("Y")) {
                    try {
                        double result = calcPostfix(postfix);
                        if (result == (long) result) System.out.println(">계산 값 : " + (long) result);
                        else System.out.println(">계산 값 : " + result);
                        break;
                    } catch (IllegalArgumentException e) {
                        System.out.println("잘못된 입력!");
                        continue;
                    }
                } else if (go.equals("n") || go.equals("N")) break;
                else System.out.println("잘못된 입력!");
            }

            while (true) {
                System.out.print("\n계속하시겠습니까? (Y/N)\n>");
                String go = sc.nextLine().trim();

                if (go.equals("y") || go.equals("Y")) break;
                else if (go.equals("n") || go.equals("N")) {
                    System.out.println("\n사용해주셔서 감사합니다.");
                    System.out.println("프로그램을 종료합니다.");
                    System.out.println("=============================");
                    still = false;
                    break;
                } else {
                    System.out.println("잘못된 입력!");
                }
            }
        }
    }
}