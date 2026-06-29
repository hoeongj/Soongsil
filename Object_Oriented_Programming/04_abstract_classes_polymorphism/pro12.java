abstract class PairMap {
    protected String keyArray[];    // 키 문자열을 저장하는 배열
    protected String valueArray[];  // 값 문자열을 저장하는 배열
    abstract public String get(String key);
    abstract public void put(String key, String value);
    abstract public String delete(String key);
    abstract public int length();
}

class Dictionary extends PairMap {
    private int size = 0;

    public Dictionary() {
        super.keyArray = new String[1];
        super.valueArray = new String[1];
    }

    public Dictionary(int capacity) {
        super.keyArray = new String[capacity];
        super.valueArray = new String[capacity];
    }

    public void enCapacity() {
        String[] newKey = new String[keyArray.length + 1];
        String[] newValue = new String[valueArray.length + 1];
        for (int i = 0; i < size; i++) {
            newKey[i] = keyArray[i];
            newValue[i] = valueArray[i];
        }
        keyArray = newKey;
        valueArray = newValue;
    }

    public void deCapacity() {
        String[] newKey = new String[keyArray.length - 1];
        String[] newValue = new String[valueArray.length - 1];
        for (int i = 0; i < size; i++) {
            newKey[i] = keyArray[i];
            newValue[i] = valueArray[i];
        }
        keyArray = newKey;
        valueArray = newValue;
    }

    public String get(String key) {
        for (int i = 0; i < size; i++) {
            if (keyArray[i].equals(key)) return valueArray[i];
        }
        return null;
    }

    public void put(String key, String value) {
        for (int i = 0; i < size; i++) {
            if (keyArray[i].equals(key)) {
                valueArray[i] = value;
                return;
            }
        }

        if (size >= keyArray.length) enCapacity();
        keyArray[size] = key;
        valueArray[size] = value;
        size++;
    }

    public String delete(String key) {
        for (int i = 0; i < size; i++) {
            if (keyArray[i].equals(key)) {
                String deletedValue = valueArray[i];

                for (int j = i; j < size - 1; j++) {
                    keyArray[j] = keyArray[j + 1];
                    valueArray[j] = valueArray[j + 1];
                }
                keyArray[size - 1] = null;
                valueArray[size - 1] = null;
                size--;
                deCapacity();
                return deletedValue;
            }
        }
        return null;
    }

    public int length() {
        return size;
    }
}

public class pro12{
    public static void main(String[] args){
        Dictionary dic = new Dictionary(10);
        dic.put("황기태","자바");
        dic.put("이재문","파이선");
        dic.put("이재문","C++");
        System.out.println("이재문의 값은 " + dic.get("이재문"));
        System.out.println("황기태의 값은 " + dic.get("황기태"));
        dic.delete("황기태");
        System.out.print("황기태의 값은 " + dic.get("황기태"));
    }
}