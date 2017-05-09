// automatically generated, do not modify

package UnrealCoojaMsg;

public final class MsgType {
  private MsgType() { }
  public static final byte LED = 1;
  public static final byte LOCATION = 2;
  public static final byte RADIO = 3;
  public static final byte PIR = 4;
  public static final byte PAUSE = 5;
  public static final byte RESUME = 6;
  public static final byte SPEED_NORM = 7;
  public static final byte SPEED_SLOW = 8;
  public static final byte SPEED_FAST = 9;
  public static final byte RADIO_DUTY = 10;

  private static final String[] names = { "LED", "LOCATION", "RADIO", "PIR", "PAUSE", "RESUME", "SPEED_NORM", "SPEED_SLOW", "SPEED_FAST", "RADIO_DUTY", };

  public static String name(int e) { return names[e - LED]; }
};

