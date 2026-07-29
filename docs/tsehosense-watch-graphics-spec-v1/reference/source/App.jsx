import { useEffect, useMemo, useRef, useState } from "react";
import {
  ArrowClockwise,
  ArrowSquareOut,
  BatteryMedium,
  BellSimple,
  Broadcast,
  CaretLeft,
  CaretRight,
  ChatCircle,
  CheckCircle,
  CircleNotch,
  Cloud,
  Gauge,
  Hexagon,
  HandTap,
  Info,
  ListBullets,
  LockKey,
  MapPin,
  Minus,
  Plus,
  Power,
  Radio,
  ShieldWarning,
  SlidersHorizontal,
  StopCircle,
  UserSwitch,
  Warning,
  WarningOctagon,
  Waveform,
  WifiSlash,
  X,
  XCircle,
} from "@phosphor-icons/react";

const demoState = {
  time: "20:42",
  battery: 78,
  temperature: 18,
  currentDevice: "AE-12",
  deviceState: "доступен",
  nearbyCount: 3,
  phoneUnread: 2,
  ecosystemUnread: 1,
};

const nearbyDevices = [
  {
    id: "AE-12",
    kind: "АКУСТИЧЕСКАЯ ЭМИССИЯ",
    location: "ПОДШИПНИК 2",
    state: "НОРМА",
    primary: [
      { label: "ТЕМПЕРАТУРА", value: "51°" },
      { label: "AE УРОВЕНЬ", value: "42 dB" },
    ],
    parameters: [
      { label: "ТЕМПЕРАТУРА", value: "51°", state: "НОРМА" },
      { label: "AE RMS", value: "42 dB", state: "НОРМА" },
      { label: "ИМПУЛЬСЫ", value: "3/мин", state: "НОРМА" },
      { label: "СВЯЗЬ", value: "−62 dBm", state: "ХОРОШО" },
    ],
  },
  {
    id: "SIG-4",
    kind: "ОБНАРУЖИТЕЛЬ СИГНАЛОВ",
    location: "ВОРОТА",
    state: "СЛУШАЕТ",
    primary: [
      { label: "ДИАПАЗОН", value: "433M" },
      { label: "СИГНАЛЫ", value: "0" },
    ],
    parameters: [
      { label: "ЧАСТОТА", value: "433.92", state: "MHz" },
      { label: "ПОРОГ", value: "−78 dBm", state: "АКТИВЕН" },
      { label: "СОБЫТИЯ", value: "0", state: "СЕГОДНЯ" },
      { label: "СВЯЗЬ", value: "LoRa", state: "ХОРОШО" },
    ],
  },
  {
    id: "PUMP-2",
    kind: "ПРИВОД НАСОСА",
    location: "КОНТУР 1",
    state: "РАБОТАЕТ",
    primary: [
      { label: "ТОК", value: "4.8 A" },
      { label: "ДАВЛЕНИЕ", value: "2.4 bar" },
    ],
    parameters: [
      { label: "ТОК", value: "4.8 A", state: "НОРМА" },
      { label: "ДАВЛЕНИЕ", value: "2.4 bar", state: "НОРМА" },
      { label: "ОБОРОТЫ", value: "1460", state: "об/мин" },
      { label: "РЕЖИМ", value: "АВТО", state: "АКТИВЕН" },
    ],
  },
];

const defaultSig4Config = {
  threshold: -78,
  band: "433.92",
  observing: false,
};

const defaultPumpConfig = {
  mode: "AUTO",
  running: true,
};

const sig4Bands = [
  { value: "315", label: "315 MHz", note: "НИЖНИЙ" },
  { value: "433.92", label: "433.92 MHz", note: "ОСНОВНОЙ" },
  { value: "868", label: "868 MHz", note: "ДАЛЬНИЙ" },
];

const commandOutcomeOptions = [
  { value: "success", label: "УСПЕХ" },
  { value: "error", label: "ОШИБКА" },
  { value: "offline", label: "НЕТ СВЯЗИ" },
];

const criticalOutcomeOptions = [
  { value: "waiting", label: "ОЖИДАЕТ" },
  { value: "approved", label: "РАЗРЕШЕНО" },
  { value: "denied", label: "ОТКЛОНЕНО" },
];

const ecosystemEvents = [
  {
    id: "ae-threshold",
    severity: "important",
    severityLabel: "ВАЖНО",
    deviceId: "AE-12",
    title: "ПОРОГ AE ПРЕВЫШЕН",
    message: "Уровень акустической эмиссии выше рабочего порога.",
    value: "68 dB",
    threshold: "порог 60 dB",
    time: "20:42",
    status: "НЕ ПРОЧИТАНО",
    unread: true,
  },
  {
    id: "signal-found",
    severity: "ordinary",
    severityLabel: "СОБЫТИЕ",
    deviceId: "SIG-4",
    title: "ОБНАРУЖЕН СИГНАЛ",
    message: "Зафиксирован сигнал в наблюдаемом диапазоне.",
    value: "433.92 MHz",
    threshold: "−61 dBm",
    time: "20:31",
    status: "ДОСТАВЛЕНО",
    unread: false,
  },
  {
    id: "pump-overheat",
    severity: "blocking",
    severityLabel: "ТРЕБУЕТ ДЕЙСТВИЯ",
    deviceId: "PUMP-2",
    title: "ПЕРЕГРЕВ ДВИГАТЕЛЯ",
    message: "Температура продолжает расти. Требуется подтверждение оператора.",
    value: "96°",
    threshold: "порог 85°",
    time: "19:56",
    status: "КВИТИРОВАНО",
    unread: false,
  },
  {
    id: "passport-sync",
    severity: "background",
    severityLabel: "ФОНОВОЕ",
    deviceId: "AE-12",
    title: "ПАСПОРТ ОБНОВЛЁН",
    message: "Шлюз получил новую версию паспорта датчика.",
    value: "v.14",
    threshold: "синхронизировано",
    time: "18:10",
    status: "В ЖУРНАЛЕ",
    unread: false,
  },
];

const severityMeta = {
  background: { label: "ФОНОВОЕ", Icon: Info },
  ordinary: { label: "ОБЫЧНОЕ", Icon: BellSimple },
  important: { label: "ВАЖНОЕ", Icon: Warning },
  blocking: { label: "БЛОКИРУЮЩЕЕ", Icon: WarningOctagon },
};

function useClock(enabled) {
  const [time, setTime] = useState(demoState.time);

  useEffect(() => {
    if (!enabled) {
      setTime(demoState.time);
      return undefined;
    }

    const tick = () => {
      setTime(
        new Intl.DateTimeFormat("ru-RU", {
          hour: "2-digit",
          minute: "2-digit",
          hour12: false,
        }).format(new Date()),
      );
    };

    tick();
    const timer = window.setInterval(tick, 1000);
    return () => window.clearInterval(timer);
  }, [enabled]);

  return time;
}

function useHorizontalSwipe({ onLeft, onRight }) {
  const startX = useRef(null);

  return {
    onPointerDown: (event) => {
      startX.current = event.clientX;
    },
    onPointerCancel: () => {
      startX.current = null;
    },
    onPointerUp: (event) => {
      if (startX.current === null) {
        return;
      }

      const distance = event.clientX - startX.current;
      startX.current = null;

      if (distance <= -44 && onLeft) {
        onLeft();
      }

      if (distance >= 44 && onRight) {
        onRight();
      }
    },
  };
}

function EcosystemMark({ size = 24 }) {
  return (
    <span className="ecosystem-mark" style={{ width: size, height: size }}>
      <Hexagon size={size} weight="regular" aria-hidden="true" />
      <span className="ecosystem-mark__dot" />
    </span>
  );
}

function HomeFace({ shape, mode, time, onOpen }) {
  const isAmbient = mode === "ambient";
  const swipe = useHorizontalSwipe({ onLeft: () => onOpen("nearby") });

  return (
    <section
      className={`watch-face watch-face--${shape} watch-face--${mode}`}
      aria-label={`Циферблат Строгий контекст, ${shape === "round" ? "круглый" : "квадратный"} профиль`}
      data-testid="watch-face"
      {...swipe}
    >
      <button
        className="time-zone"
        type="button"
        onClick={() => onOpen("clock")}
        aria-label={`Время ${time}`}
      >
        <span className="time">{time}</span>
      </button>

      {!isAmbient && (
        <>
          <div className="hairline hairline--top" />

          <div className="status-row" aria-label="Состояние часов">
            <button
              className="status-cell status-cell--battery"
              type="button"
              onClick={() => onOpen("battery")}
              aria-label={`Заряд ${demoState.battery} процентов`}
            >
              <BatteryMedium size={32} weight="regular" aria-hidden="true" />
              <span>{demoState.battery}%</span>
            </button>
            <span className="status-divider" />
            <button
              className="status-cell status-cell--weather"
              type="button"
              onClick={() => onOpen("weather")}
              aria-label={`Погода ${demoState.temperature} градусов`}
            >
              <Cloud size={32} weight="regular" aria-hidden="true" />
              <span>{demoState.temperature}°</span>
            </button>
          </div>

          <div className="hairline hairline--middle" />

          <button
            className="device-zone"
            type="button"
            onClick={() => onOpen("device")}
            aria-label={`${demoState.currentDevice}, ${demoState.deviceState}`}
          >
            <span className="device-copy">
              <strong>{demoState.currentDevice}</strong>
              <small>{demoState.deviceState}</small>
            </span>
            <CaretRight className="device-chevron" size={60} weight="bold" aria-hidden="true" />
          </button>

          <div className="bottom-row">
            <button
              className="counter counter--phone"
              type="button"
              onClick={() => onOpen("phone")}
              aria-label={`Телефон, ${demoState.phoneUnread} непрочитанных`}
            >
              <ChatCircle size={27} weight="regular" aria-hidden="true" />
              <span>{demoState.phoneUnread}</span>
            </button>
            <button
              className="nearby"
              type="button"
              onClick={() => onOpen("nearby")}
              aria-label={`${demoState.nearbyCount} устройства рядом`}
            >
              {demoState.nearbyCount} рядом
            </button>
            <button
              className="counter counter--ecosystem"
              type="button"
              onClick={() => onOpen("ecosystem")}
              aria-label={`Экосистема, ${demoState.ecosystemUnread} непрочитанное событие`}
            >
              <EcosystemMark size={25} />
              <span>{demoState.ecosystemUnread}</span>
            </button>
          </div>
        </>
      )}

      {isAmbient && (
        <div className="ambient-markers" aria-label="Непрочитанные события">
          <span>
            <ChatCircle size={17} weight="regular" aria-hidden="true" />
            {demoState.phoneUnread}
          </span>
          <span>
            <EcosystemMark size={17} />
            {demoState.ecosystemUnread}
          </span>
        </div>
      )}
    </section>
  );
}

function ScreenHeader({ eyebrow, title, onBack }) {
  return (
    <header className="screen-header">
      <button className="screen-back" type="button" onClick={onBack} aria-label="Назад">
        <CaretLeft size={25} weight="regular" aria-hidden="true" />
      </button>
      <span className="screen-header__copy">
        <small>{eyebrow}</small>
        <strong>{title}</strong>
      </span>
    </header>
  );
}

function DeviceList({ shape, onBack, onSelect }) {
  const swipe = useHorizontalSwipe({ onRight: onBack });

  return (
    <section
      className={`watch-face device-list-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label="3 устройства рядом"
      {...swipe}
    >
      <ScreenHeader eyebrow="УСТРОЙСТВА" title="3 РЯДОМ" onBack={onBack} />
      <div className="device-list">
        {nearbyDevices.map((device) => (
          <button
            key={device.id}
            className="device-list-row"
            type="button"
            onClick={() => onSelect(device.id)}
            aria-label={`${device.id}, ${device.state}, ${device.location}`}
          >
            <span className="device-list-row__main">
              <strong>{device.id}</strong>
              <small>
                <MapPin size={13} weight="regular" aria-hidden="true" />
                {device.location}
              </small>
            </span>
            <span className="device-list-row__state">{device.state}</span>
            <CaretRight size={20} weight="regular" aria-hidden="true" />
          </button>
        ))}
      </div>
    </section>
  );
}

function DeviceOverview({ shape, device, onBack, onParameters, onControl }) {
  const swipe = useHorizontalSwipe({ onRight: onBack });

  return (
    <section
      className={`watch-face device-overview-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label={`${device.id}, ${device.state}`}
      {...swipe}
    >
      <ScreenHeader eyebrow={device.kind} title={device.id} onBack={onBack} />

      <div className="device-health" aria-label={`Состояние ${device.state}`}>
        <CheckCircle size={25} weight="regular" aria-hidden="true" />
        <strong>{device.state}</strong>
        <small>{device.location}</small>
      </div>

      <div className="primary-metrics">
        {device.primary.map((metric) => (
          <button
            key={metric.label}
            className="primary-metric"
            type="button"
            onClick={onParameters}
            aria-label={`${metric.label}, ${metric.value}`}
          >
            <small>{metric.label}</small>
            <strong>{metric.value}</strong>
          </button>
        ))}
      </div>

      {onControl ? (
        <div className="device-actions">
          <button type="button" onClick={onParameters}>
            <ListBullets size={19} weight="regular" aria-hidden="true" />
            <span>ПАРАМЕТРЫ</span>
          </button>
          <button type="button" onClick={onControl}>
            <SlidersHorizontal size={19} weight="regular" aria-hidden="true" />
            <span>УПРАВЛЕНИЕ</span>
          </button>
        </div>
      ) : (
        <button className="all-parameters" type="button" onClick={onParameters}>
          <ListBullets size={21} weight="regular" aria-hidden="true" />
          <span>ВСЕ ПАРАМЕТРЫ</span>
          <strong>{device.parameters.length}</strong>
          <CaretRight size={19} weight="regular" aria-hidden="true" />
        </button>
      )}
    </section>
  );
}

function ParameterTable({ shape, device, onBack }) {
  const swipe = useHorizontalSwipe({ onRight: onBack });

  return (
    <section
      className={`watch-face parameter-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label={`Параметры ${device.id}`}
      {...swipe}
    >
      <ScreenHeader eyebrow={device.id} title="ПАРАМЕТРЫ" onBack={onBack} />
      <div className="parameter-list" tabIndex="0" aria-label="Прокручиваемый список параметров">
        {device.parameters.map((parameter) => (
          <button className="parameter-row" type="button" key={parameter.label}>
            <span className="parameter-row__label">
              <Waveform size={17} weight="regular" aria-hidden="true" />
              {parameter.label}
            </span>
            <strong>{parameter.value}</strong>
            <small>{parameter.state}</small>
          </button>
        ))}
      </div>
      <span className="scroll-hint" aria-hidden="true">КРУТИТЬ</span>
    </section>
  );
}

function Sig4Control({ shape, config, onBack, onThreshold, onBand, onObservation }) {
  const swipe = useHorizontalSwipe({ onRight: onBack });

  return (
    <section
      className={`watch-face control-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label="Управление SIG-4"
      {...swipe}
    >
      <ScreenHeader eyebrow="SIG-4" title="УПРАВЛЕНИЕ" onBack={onBack} />

      <button
        className={`observation-control ${config.observing ? "is-on" : ""}`}
        type="button"
        onClick={() => onObservation(!config.observing)}
        aria-pressed={config.observing}
      >
        <Broadcast size={28} weight={config.observing ? "fill" : "regular"} aria-hidden="true" />
        <span>
          <small>НАБЛЮДЕНИЕ</small>
          <strong>{config.observing ? "ВКЛЮЧЕНО" : "ВЫКЛЮЧЕНО"}</strong>
        </span>
        <Power size={22} weight="regular" aria-hidden="true" />
      </button>

      <div className="control-settings">
        <button type="button" onClick={onThreshold}>
          <Gauge size={20} weight="regular" aria-hidden="true" />
          <span>
            <small>ПОРОГ</small>
            <strong>{config.threshold} dBm</strong>
          </span>
          <CaretRight size={18} weight="regular" aria-hidden="true" />
        </button>
        <button type="button" onClick={onBand}>
          <Radio size={20} weight="regular" aria-hidden="true" />
          <span>
            <small>ДИАПАЗОН</small>
            <strong>{config.band}M</strong>
          </span>
          <CaretRight size={18} weight="regular" aria-hidden="true" />
        </button>
      </div>

      <small className="control-safety-note">БЕЗОПАСНЫЕ КОМАНДЫ · БЕЗ ПОДТВЕРЖДЕНИЯ</small>
    </section>
  );
}

function ThresholdEditor({ shape, value, onBack, onApply }) {
  const [draft, setDraft] = useState(value);
  const swipe = useHorizontalSwipe({ onRight: onBack });
  const change = (delta) => setDraft((current) => Math.min(-30, Math.max(-110, current + delta)));

  return (
    <section
      className={`watch-face threshold-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label={`Порог SIG-4, ${draft} dBm`}
      {...swipe}
    >
      <ScreenHeader eyebrow="SIG-4" title="ПОРОГ" onBack={onBack} />

      <div className="threshold-value">
        <strong>{draft}</strong>
        <span>dBm</span>
      </div>

      <div className="threshold-stepper">
        <button type="button" onClick={() => change(-1)} aria-label="Уменьшить порог на 1 dBm">
          <Minus size={28} weight="regular" aria-hidden="true" />
        </button>
        <span>
          <small>ШАГ</small>
          <strong>1 dBm</strong>
        </span>
        <button type="button" onClick={() => change(1)} aria-label="Увеличить порог на 1 dBm">
          <Plus size={28} weight="regular" aria-hidden="true" />
        </button>
      </div>

      <span className="threshold-range">−110 … −30 dBm</span>
      <button className="command-apply" type="button" onClick={() => onApply(draft)}>
        ПРИМЕНИТЬ
      </button>
    </section>
  );
}

function BandSelector({ shape, value, onBack, onApply }) {
  const [draft, setDraft] = useState(value);
  const swipe = useHorizontalSwipe({ onRight: onBack });

  return (
    <section
      className={`watch-face band-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label={`Диапазон SIG-4, ${draft} MHz`}
      {...swipe}
    >
      <ScreenHeader eyebrow="SIG-4" title="ДИАПАЗОН" onBack={onBack} />
      <div className="band-options" role="radiogroup" aria-label="Рабочий диапазон">
        {sig4Bands.map((band) => (
          <button
            type="button"
            role="radio"
            aria-checked={draft === band.value}
            className={draft === band.value ? "is-selected" : ""}
            key={band.value}
            onClick={() => setDraft(band.value)}
          >
            <span className="band-check">
              {draft === band.value && <CheckCircle size={20} weight="fill" aria-hidden="true" />}
            </span>
            <strong>{band.label}</strong>
            <small>{band.note}</small>
          </button>
        ))}
      </div>
      <button className="command-apply" type="button" onClick={() => onApply(draft)}>
        ПРИМЕНИТЬ
      </button>
    </section>
  );
}

function PumpControl({ shape, config, onBack, onMode, onStop }) {
  const swipe = useHorizontalSwipe({ onRight: onBack });

  return (
    <section
      className={`watch-face pump-control-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label="Управление PUMP-2"
      {...swipe}
    >
      <ScreenHeader eyebrow="PUMP-2" title="УПРАВЛЕНИЕ" onBack={onBack} />
      <div className="pump-confirmed-state">
        <CheckCircle size={26} weight="regular" aria-hidden="true" />
        <span>
          <small>ПОДТВЕРЖДЕНО УСТРОЙСТВОМ</small>
          <strong>{config.running ? "РАБОТАЕТ" : "ОСТАНОВЛЕН"}</strong>
        </span>
      </div>
      <div className="pump-actions">
        <button type="button" onClick={onMode}>
          <UserSwitch size={23} weight="regular" aria-hidden="true" />
          <span>
            <small>ЗАЩИЩЁННАЯ</small>
            <strong>РЕЖИМ · {config.mode === "AUTO" ? "АВТО" : "РУЧНОЙ"}</strong>
          </span>
          <CaretRight size={19} weight="regular" aria-hidden="true" />
        </button>
        <button type="button" className="is-critical" onClick={onStop}>
          <StopCircle size={23} weight="regular" aria-hidden="true" />
          <span>
            <small>КРИТИЧЕСКАЯ</small>
            <strong>ОСТАНОВ</strong>
          </span>
          <CaretRight size={19} weight="regular" aria-hidden="true" />
        </button>
      </div>
      <small className="control-safety-note">РИСК ОПРЕДЕЛЯЕТ СПОСОБ ПОДТВЕРЖДЕНИЯ</small>
    </section>
  );
}

function PumpModeSelector({ shape, value, onBack, onSelect }) {
  const swipe = useHorizontalSwipe({ onRight: onBack });

  return (
    <section
      className={`watch-face pump-mode-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label="Выбор режима PUMP-2"
      {...swipe}
    >
      <ScreenHeader eyebrow="PUMP-2" title="РЕЖИМ" onBack={onBack} />
      <div className="pump-mode-current">
        <small>ТЕКУЩИЙ РЕЖИМ</small>
        <strong>{value === "AUTO" ? "АВТО" : "РУЧНОЙ"}</strong>
      </div>
      <div className="pump-mode-options">
        {[
          ["AUTO", "АВТО", "РЕГУЛИРУЕТ АВТОМАТИКА"],
          ["MANUAL", "РУЧНОЙ", "ОТВЕТСТВЕННОСТЬ ОПЕРАТОРА"],
        ].map(([mode, label, note]) => (
          <button
            type="button"
            key={mode}
            className={value === mode ? "is-current" : ""}
            onClick={() => onSelect(mode)}
          >
            {value === mode ? (
              <CheckCircle size={21} weight="fill" aria-hidden="true" />
            ) : (
              <UserSwitch size={21} weight="regular" aria-hidden="true" />
            )}
            <span>
              <strong>{label}</strong>
              <small>{note}</small>
            </span>
            <CaretRight size={18} weight="regular" aria-hidden="true" />
          </button>
        ))}
      </div>
    </section>
  );
}

function GuardedConfirmation({ shape, fromMode, toMode, onBack, onConfirm, initialProgress = 0 }) {
  const [progress, setProgress] = useState(initialProgress);
  const timer = useRef(null);
  const startedAt = useRef(0);

  const cancelHold = () => {
    if (timer.current) {
      window.clearInterval(timer.current);
      timer.current = null;
    }
    setProgress(0);
  };

  const beginHold = (event) => {
    event.preventDefault();
    if (timer.current) {
      return;
    }
    if (event.pointerId !== undefined) {
      event.currentTarget.setPointerCapture?.(event.pointerId);
    }
    startedAt.current = performance.now();
    timer.current = window.setInterval(() => {
      const next = Math.min(1, (performance.now() - startedAt.current) / 1500);
      setProgress(next);
      if (next >= 1) {
        window.clearInterval(timer.current);
        timer.current = null;
        onConfirm();
      }
    }, 40);
  };

  useEffect(() => () => timer.current && window.clearInterval(timer.current), []);

  return (
    <section
      className={`watch-face guarded-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label="Подтверждение защищённой команды PUMP-2"
    >
      <ScreenHeader eyebrow="PUMP-2" title="ЗАЩИЩЕНО" onBack={onBack} />
      <div className="guarded-heading">
        <LockKey size={28} weight="regular" aria-hidden="true" />
        <span>
          <small>СМЕНА РЕЖИМА</small>
          <strong>{toMode === "MANUAL" ? "РУЧНОЙ" : "АВТО"}</strong>
        </span>
      </div>
      <div className="mode-transition">
        <span><small>СЕЙЧАС</small><strong>{fromMode === "AUTO" ? "АВТО" : "РУЧНОЙ"}</strong></span>
        <CaretRight size={21} weight="regular" aria-hidden="true" />
        <span><small>БУДЕТ</small><strong>{toMode === "AUTO" ? "АВТО" : "РУЧНОЙ"}</strong></span>
      </div>
      <p>{toMode === "MANUAL" ? "Автоматика перестанет регулировать привод." : "Управление вернётся автоматике."}</p>
      <button
        className="hold-confirm"
        type="button"
        onPointerDown={beginHold}
        onPointerUp={cancelHold}
        onPointerLeave={cancelHold}
        onPointerCancel={cancelHold}
        onKeyDown={(event) => {
          if ((event.key === " " || event.key === "Enter") && !event.repeat) beginHold(event);
        }}
        onKeyUp={(event) => {
          if (event.key === " " || event.key === "Enter") cancelHold();
        }}
        aria-label="Удерживать полторы секунды, чтобы подтвердить смену режима"
      >
        <span className="hold-confirm__fill" style={{ width: `${progress * 100}%` }} />
        <HandTap size={21} weight="regular" aria-hidden="true" />
        <strong>{progress > 0 ? `${Math.round(progress * 100)}%` : "УДЕРЖИВАТЬ 1,5 С"}</strong>
      </button>
    </section>
  );
}

function CriticalStopRequest({ shape, onBack, onRequest }) {
  return (
    <section
      className={`watch-face critical-stop-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label="Критический останов PUMP-2"
    >
      <ScreenHeader eyebrow="PUMP-2" title="КРИТИЧЕСКАЯ" onBack={onBack} />
      <div className="critical-stop-heading">
        <ShieldWarning size={31} weight="regular" aria-hidden="true" />
        <strong>ОСТАНОВИТЬ НАСОС</strong>
      </div>
      <p>Контур потеряет давление. Часы не могут выполнить эту команду напрямую.</p>
      <div className="critical-authority">
        <LockKey size={20} weight="regular" aria-hidden="true" />
        <span><small>ТРЕБУЕТСЯ</small><strong>ВНЕШНЕЕ РАЗРЕШЕНИЕ</strong></span>
      </div>
      <button className="critical-request-button" type="button" onClick={onRequest}>
        ЗАПРОСИТЬ РАЗРЕШЕНИЕ
      </button>
    </section>
  );
}

const commandPhaseMeta = {
  sending: {
    label: "ОТПРАВКА",
    detail: "КОМАНДА УХОДИТ ЧЕРЕЗ LoRa",
    Icon: Radio,
    step: 1,
  },
  accepted: {
    label: "ПРИНЯТА",
    detail: "SIG-4 ПОДТВЕРДИЛ ПАКЕТ",
    Icon: CheckCircle,
    step: 2,
  },
  executing: {
    label: "ВЫПОЛНЯЕТСЯ",
    detail: "УСТРОЙСТВО МЕНЯЕТ РЕЖИМ",
    Icon: CircleNotch,
    step: 3,
  },
  success: {
    label: "ГОТОВО",
    detail: "СОСТОЯНИЕ ПОДТВЕРЖДЕНО",
    Icon: CheckCircle,
    step: 4,
  },
  error: {
    label: "ОШИБКА",
    detail: "УСТРОЙСТВО ОТКЛОНИЛО КОМАНДУ",
    Icon: XCircle,
    step: 3,
  },
  offline: {
    label: "НЕТ СВЯЗИ",
    detail: "SIG-4 НЕ ПОДТВЕРДИЛ КОМАНДУ",
    Icon: WifiSlash,
    step: 1,
  },
};

function CommandOverlay({ shape, command, onRetry, onDismiss }) {
  if (!command) {
    return null;
  }

  const meta = commandPhaseMeta[command.phase] ?? commandPhaseMeta.sending;
  const Icon = meta.Icon;
  const failed = command.phase === "error" || command.phase === "offline";

  return (
    <section
      className={`command-overlay command-overlay--${command.phase} watch-face--${shape}`}
      aria-label={`${meta.label}: ${command.label}`}
    >
      <div className="command-state-icon">
        <Icon
          size={45}
          weight={command.phase === "success" ? "fill" : "regular"}
          aria-hidden="true"
        />
      </div>
      <small>{command.sourceId ?? "УСТРОЙСТВО"} · КОМАНДА</small>
      <strong className="command-state-label">{meta.label}</strong>
      <span className="command-state-title">{command.label}</span>
      <div className="command-progress" aria-label={`Шаг ${meta.step} из 4`}>
        {[1, 2, 3, 4].map((step) => (
          <span key={step} className={step <= meta.step ? "is-complete" : ""} />
        ))}
      </div>
      <p>{meta.detail.replace("SIG-4", command.sourceId ?? "УСТРОЙСТВО")}</p>

      {failed && (
        <div className="command-failure-actions">
          <button type="button" onClick={onDismiss}>НАЗАД</button>
          <button type="button" onClick={onRetry}>
            <ArrowClockwise size={18} weight="regular" aria-hidden="true" />
            ПОВТОРИТЬ
          </button>
        </div>
      )}
    </section>
  );
}

const criticalPhaseMeta = {
  sending: ["ЗАПРОС УХОДИТ", "ШЛЮЗ ПЕРЕДАЁТ ВО ВНЕШНИЙ КОНТУР", Radio],
  waiting: ["ОЖИДАЕТ РЕШЕНИЯ", "КОМАНДА НЕ ВЫПОЛНЯЕТСЯ", CircleNotch],
  approved: ["РАЗРЕШЕНО", "ВЫПОЛНИТЬ МОЖНО ТОЛЬКО С ВНЕШНЕГО ПУЛЬТА", CheckCircle],
  denied: ["ОТКЛОНЕНО", "ПРАВО НА ОСТАНОВ НЕ ПРЕДОСТАВЛЕНО", XCircle],
};

function CriticalRequestOverlay({ shape, request, onCancel, onDismiss }) {
  if (!request) {
    return null;
  }
  const [label, detail, Icon] = criticalPhaseMeta[request.phase] ?? criticalPhaseMeta.sending;
  const terminal = request.phase === "approved" || request.phase === "denied";

  return (
    <section
      className={`critical-request-overlay critical-request-overlay--${request.phase} watch-face--${shape}`}
      aria-label={`Критический запрос: ${label}`}
    >
      <div className="critical-request-icon">
        <Icon size={46} weight={request.phase === "approved" ? "fill" : "regular"} aria-hidden="true" />
      </div>
      <small>PUMP-2 · КРИТИЧЕСКИЙ ЗАПРОС</small>
      <strong>{label}</strong>
      <span>ОСТАНОВИТЬ НАСОС</span>
      <p>{detail}</p>
      {request.phase === "waiting" && (
        <button type="button" onClick={onCancel}>ОТМЕНИТЬ ЗАПРОС</button>
      )}
      {terminal && (
        <button type="button" onClick={onDismiss}>ЗАКРЫТЬ</button>
      )}
    </section>
  );
}

function SeverityIcon({ severity, size = 22 }) {
  const { Icon } = severityMeta[severity] ?? severityMeta.background;
  return <Icon size={size} weight="regular" aria-hidden="true" />;
}

function EventJournal({ shape, onBack, onOpen }) {
  const swipe = useHorizontalSwipe({ onRight: onBack });

  return (
    <section
      className={`watch-face event-journal-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label="Журнал событий экосистемы"
      {...swipe}
    >
      <ScreenHeader eyebrow="ЭКОСИСТЕМА" title="СОБЫТИЯ 4" onBack={onBack} />
      <div className="event-list" tabIndex="0" aria-label="Прокручиваемый журнал событий">
        {ecosystemEvents.map((event) => (
          <button
            className={`event-list-row event-list-row--${event.severity}`}
            type="button"
            key={event.id}
            onClick={() => onOpen(event.id)}
            aria-label={`${event.deviceId}, ${event.title}, ${event.severityLabel}, ${event.time}`}
          >
            <span className="event-list-row__icon">
              <SeverityIcon severity={event.severity} size={20} />
            </span>
            <span className="event-list-row__copy">
              <strong>{event.deviceId}</strong>
              <small>{event.title}</small>
            </span>
            <span className="event-list-row__meta">
              <small>{event.time}</small>
              <strong>{event.severityLabel}</strong>
            </span>
            <CaretRight size={18} weight="regular" aria-hidden="true" />
          </button>
        ))}
      </div>
      <span className="scroll-hint" aria-hidden="true">КРУТИТЬ</span>
    </section>
  );
}

function EventDetail({ shape, event, onBack, onOpenDevice }) {
  const swipe = useHorizontalSwipe({ onRight: onBack });

  return (
    <section
      className={`watch-face event-detail-screen watch-face--${shape}`}
      data-testid="watch-face"
      aria-label={`${event.deviceId}, ${event.title}`}
      {...swipe}
    >
      <ScreenHeader eyebrow={event.deviceId} title={event.severityLabel} onBack={onBack} />
      <div className="event-detail-title">
        <SeverityIcon severity={event.severity} size={27} />
        <strong>{event.title}</strong>
      </div>
      <p className="event-detail-message">{event.message}</p>
      <div className="event-detail-value">
        <strong>{event.value}</strong>
        <small>{event.threshold}</small>
      </div>
      <div className="event-detail-meta">
        <span>{event.time}</span>
        <strong>{event.status}</strong>
      </div>
      <button className="event-open-device" type="button" onClick={onOpenDevice}>
        <ArrowSquareOut size={19} weight="regular" aria-hidden="true" />
        <span>ОТКРЫТЬ {event.deviceId}</span>
        <CaretRight size={18} weight="regular" aria-hidden="true" />
      </button>
    </section>
  );
}

function EventOverlay({ shape, event, onDismiss, onOpenDevice, onAcknowledge }) {
  if (!event || event.severity === "background") {
    return null;
  }

  if (event.severity === "ordinary") {
    return (
      <button
        className={`event-toast watch-face--${shape}`}
        type="button"
        onClick={onOpenDevice}
        aria-label={`${event.deviceId}, ${event.title}. Открыть устройство`}
      >
        <span className="event-toast__icon">
          <BellSimple size={22} weight="regular" aria-hidden="true" />
        </span>
        <span className="event-toast__copy">
          <strong>{event.deviceId}</strong>
          <small>{event.title}</small>
        </span>
        <CaretRight size={20} weight="regular" aria-hidden="true" />
      </button>
    );
  }

  if (event.severity === "important") {
    return (
      <section
        className={`event-overlay event-overlay--important watch-face--${shape}`}
        aria-label={`Важное событие: ${event.title}`}
      >
        <button className="event-overlay-dismiss" type="button" onClick={onDismiss} aria-label="Позже">
          <X size={24} weight="regular" aria-hidden="true" />
        </button>
        <div className="event-overlay-heading">
          <Warning size={31} weight="regular" aria-hidden="true" />
          <small>ВАЖНО · {event.deviceId}</small>
          <strong>{event.title}</strong>
        </div>
        <div className="event-overlay-reading">
          <strong>{event.value}</strong>
          <small>{event.threshold}</small>
        </div>
        <p>{event.message}</p>
        <div className="event-overlay-actions">
          <button type="button" onClick={onDismiss}>ПОЗЖЕ</button>
          <button type="button" onClick={onOpenDevice}>ОТКРЫТЬ</button>
        </div>
      </section>
    );
  }

  return (
    <section
      className={`event-overlay event-overlay--blocking watch-face--${shape}`}
      aria-label={`Блокирующее событие: ${event.title}`}
    >
      <div className="event-overlay-heading">
        <WarningOctagon size={34} weight="regular" aria-hidden="true" />
        <small>ТРЕБУЕТ ДЕЙСТВИЯ · {event.deviceId}</small>
        <strong>{event.title}</strong>
      </div>
      <div className="event-overlay-reading">
        <strong>{event.value}</strong>
        <small>{event.threshold}</small>
      </div>
      <p>{event.message}</p>
      <button className="blocking-acknowledge" type="button" onClick={onAcknowledge}>
        КВИТИРОВАТЬ
      </button>
    </section>
  );
}

const panels = {
  phone: {
    eyebrow: "ТЕЛЕФОН",
    title: "2 НОВЫХ",
    rows: [
      ["СООБЩЕНИЕ", "10:36"],
      ["ПРОПУЩЕННЫЙ", "09:14"],
    ],
  },
  ecosystem: {
    eyebrow: "ЭКОСИСТЕМА",
    title: "1 СОБЫТИЕ",
    rows: [
      ["SIG-4", "СИГНАЛ"],
      ["433.92 MHz", "20:42"],
    ],
  },
  battery: {
    eyebrow: "БАТАРЕЯ",
    title: "78%",
    rows: [
      ["ОЦЕНКА", "2 ДНЯ"],
      ["РЕЖИМ", "ОБЫЧНЫЙ"],
    ],
  },
  weather: {
    eyebrow: "ПОГОДА",
    title: "18°",
    rows: [
      ["СЕЙЧАС", "ОБЛАЧНО"],
      ["ВЕТЕР", "3 м/с"],
    ],
  },
  clock: {
    eyebrow: "ЧАСЫ",
    title: "20:42",
    rows: [
      ["ЧЕТВЕРГ", "23 ИЮЛЯ"],
      ["БУДИЛЬНИК", "ВЫКЛ"],
    ],
  },
};

function DetailPanel({ type, shape, onBack }) {
  const panel = panels[type] ?? panels.device;

  return (
    <section
      className={`watch-face detail-panel watch-face--${shape}`}
      data-testid="watch-face"
      aria-label={panel.title}
    >
      <button className="detail-back" type="button" onClick={onBack} aria-label="Назад">
        <CaretLeft size={26} weight="regular" aria-hidden="true" />
      </button>
      <div className="detail-heading">
        <small>{panel.eyebrow}</small>
        <strong>{panel.title}</strong>
      </div>
      <div className="detail-list">
        {panel.rows.map(([label, value]) => (
          <button key={`${label}-${value}`} type="button" className="detail-row">
            <span>{label}</span>
            <strong>{value}</strong>
            <CaretRight size={18} weight="regular" aria-hidden="true" />
          </button>
        ))}
      </div>
    </section>
  );
}

function WatchRenderer({
  shape,
  mode,
  liveTime,
  screen,
  activeEvent,
  activeCommand,
  activeCriticalRequest,
  sig4Config,
  pumpConfig,
  holdProgress,
  onNavigate,
  onReplace,
  onBack,
  onCommand,
  onRetryCommand,
  onDismissCommand,
  onCriticalRequest,
  onCancelCriticalRequest,
  onDismissCriticalRequest,
  onDismissEvent,
  onAcknowledgeEvent,
}) {
  const time = useClock(liveTime);
  let content;

  if (screen === "devices") {
    content = (
      <DeviceList
        shape={shape}
        onBack={onBack}
        onSelect={(deviceId) => onNavigate(`device:${deviceId}`)}
      />
    );
  } else if (screen.startsWith("device:")) {
    const deviceId = screen.slice("device:".length);
    const device = nearbyDevices.find((candidate) => candidate.id === deviceId) ?? nearbyDevices[0];
    content = (
      <DeviceOverview
        shape={shape}
        device={device}
        onBack={onBack}
        onParameters={() => onNavigate(`parameters:${device.id}`)}
        onControl={
          device.id === "SIG-4" || device.id === "PUMP-2"
            ? () => onNavigate(`control:${device.id}`)
            : undefined
        }
      />
    );
  } else if (screen.startsWith("parameters:")) {
    const deviceId = screen.slice("parameters:".length);
    const device = nearbyDevices.find((candidate) => candidate.id === deviceId) ?? nearbyDevices[0];
    content = <ParameterTable shape={shape} device={device} onBack={onBack} />;
  } else if (screen === "control:SIG-4") {
    content = (
      <Sig4Control
        shape={shape}
        config={sig4Config}
        onBack={onBack}
        onThreshold={() => onNavigate("threshold:SIG-4")}
        onBand={() => onNavigate("band:SIG-4")}
        onObservation={(observing) =>
          onCommand({
            kind: "observation",
            value: observing,
            label: observing ? "НАЧАТЬ НАБЛЮДЕНИЕ" : "ОСТАНОВИТЬ НАБЛЮДЕНИЕ",
          })
        }
      />
    );
  } else if (screen === "threshold:SIG-4") {
    content = (
      <ThresholdEditor
        shape={shape}
        value={sig4Config.threshold}
        onBack={onBack}
        onApply={(threshold) => {
          onReplace("control:SIG-4");
          onCommand({
            kind: "threshold",
            value: threshold,
            label: `ПОРОГ ${threshold} dBm`,
          });
        }}
      />
    );
  } else if (screen === "band:SIG-4") {
    content = (
      <BandSelector
        shape={shape}
        value={sig4Config.band}
        onBack={onBack}
        onApply={(band) => {
          onReplace("control:SIG-4");
          onCommand({
            kind: "band",
            value: band,
            label: `ДИАПАЗОН ${band} MHz`,
          });
        }}
      />
    );
  } else if (screen === "control:PUMP-2") {
    content = (
      <PumpControl
        shape={shape}
        config={pumpConfig}
        onBack={onBack}
        onMode={() => onNavigate("pump-mode:PUMP-2")}
        onStop={() => onNavigate("critical:PUMP-2")}
      />
    );
  } else if (screen === "pump-mode:PUMP-2") {
    content = (
      <PumpModeSelector
        shape={shape}
        value={pumpConfig.mode}
        onBack={onBack}
        onSelect={(nextMode) => {
          if (nextMode === pumpConfig.mode) {
            onBack();
            return;
          }
          onNavigate(`guarded:PUMP-2:${nextMode}`);
        }}
      />
    );
  } else if (screen.startsWith("guarded:PUMP-2:")) {
    const nextMode = screen.slice("guarded:PUMP-2:".length) || "MANUAL";
    content = (
      <GuardedConfirmation
        shape={shape}
        fromMode={pumpConfig.mode}
        toMode={nextMode}
        initialProgress={holdProgress}
        onBack={onBack}
        onConfirm={() => {
          onReplace("control:PUMP-2");
          onCommand({
            kind: "pumpMode",
            value: nextMode,
            label: `РЕЖИМ ${nextMode === "AUTO" ? "АВТО" : "РУЧНОЙ"}`,
            sourceId: "PUMP-2",
            transport: "LoRa",
            risk: "guarded",
          });
        }}
      />
    );
  } else if (screen === "critical:PUMP-2") {
    content = (
      <CriticalStopRequest
        shape={shape}
        onBack={onBack}
        onRequest={onCriticalRequest}
      />
    );
  } else if (screen === "events") {
    content = (
      <EventJournal
        shape={shape}
        onBack={onBack}
        onOpen={(eventId) => onNavigate(`event:${eventId}`)}
      />
    );
  } else if (screen.startsWith("event:")) {
    const eventId = screen.slice("event:".length);
    const event = ecosystemEvents.find((candidate) => candidate.id === eventId) ?? ecosystemEvents[0];
    content = (
      <EventDetail
        shape={shape}
        event={event}
        onBack={onBack}
        onOpenDevice={() => onNavigate(`device:${event.deviceId}`)}
      />
    );
  } else if (screen !== "home") {
    content = <DetailPanel type={screen} shape={shape} onBack={onBack} />;
  } else {
    content = (
      <HomeFace
        shape={shape}
        mode={mode}
        time={time}
        onOpen={(target) => {
          if (target === "nearby") {
            onNavigate("devices");
            return;
          }

          if (target === "device") {
            onNavigate(`device:${nearbyDevices[0].id}`);
            return;
          }

          if (target === "ecosystem") {
            onNavigate("events");
            return;
          }

          onNavigate(target);
        }}
      />
    );
  }

  return (
    <div className={`watch-render-stack watch-render-stack--${shape}`}>
      {content}
      <CommandOverlay
        key={`command-${activeCommand?.instanceId ?? "none"}`}
        shape={shape}
        command={activeCommand}
        onRetry={onRetryCommand}
        onDismiss={onDismissCommand}
      />
      <CriticalRequestOverlay
        key={`critical-${activeCriticalRequest?.instanceId ?? "none"}`}
        shape={shape}
        request={activeCriticalRequest}
        onCancel={onCancelCriticalRequest}
        onDismiss={onDismissCriticalRequest}
      />
      <EventOverlay
        key={`event-${activeEvent?.presentationId ?? activeEvent?.id ?? "none"}`}
        shape={shape}
        event={activeEvent}
        onDismiss={onDismissEvent}
        onAcknowledge={onAcknowledgeEvent}
        onOpenDevice={() => {
          onDismissEvent();
          onNavigate(`device:${activeEvent.deviceId}`);
        }}
      />
    </div>
  );
}

function Segmented({ label, value, options, onChange }) {
  return (
    <fieldset className="segmented">
      <legend>{label}</legend>
      <div style={{ gridTemplateColumns: `repeat(${options.length}, minmax(0, 1fr))` }}>
        {options.map((option) => (
          <button
            key={option.value}
            type="button"
            className={value === option.value ? "is-active" : ""}
            aria-pressed={value === option.value}
            onClick={() => onChange(option.value)}
          >
            {option.label}
          </button>
        ))}
      </div>
    </fieldset>
  );
}

function EventSimulator({ activeEventId, runCount, status, onTrigger }) {
  return (
    <fieldset className="event-simulator">
      <legend>Событие экосистемы</legend>
      <div className="event-simulator__grid">
        {Object.entries(severityMeta).map(([severity, meta]) => {
          const event = ecosystemEvents.find((candidate) => candidate.severity === severity);
          const Icon = meta.Icon;
          return (
            <button
              key={severity}
              type="button"
              className={activeEventId === event.id ? "is-active" : ""}
              aria-pressed={activeEventId === event.id}
              onClick={() => onTrigger(event)}
            >
              <Icon size={18} weight="regular" aria-hidden="true" />
              <span>{meta.label}</span>
            </button>
          );
        })}
      </div>
      <small key={runCount} aria-live="polite">
        {status}
      </small>
    </fieldset>
  );
}

export function App() {
  const capture = window.location.pathname === "/capture";
  const query = useMemo(() => new URLSearchParams(window.location.search), []);
  const [shape, setShape] = useState(query.get("shape") === "round" ? "round" : "square");
  const [mode, setMode] = useState(query.get("mode") === "ambient" ? "ambient" : "normal");
  const [liveTime, setLiveTime] = useState(!capture && query.get("live") === "1");
  const captureScreen = query.get("screen") ?? "home";
  const captureEventId = query.get("event");
  const captureEvent =
    ecosystemEvents.find((candidate) => candidate.id === captureEventId) ?? null;
  const captureCommandPhase = query.get("command");
  const captureCommand = captureCommandPhase
    ? {
        kind: "observation",
        value: true,
        label: query.get("commandLabel") ?? "НАЧАТЬ НАБЛЮДЕНИЕ",
        phase: captureCommandPhase,
        instanceId: "capture-command",
        sourceId: query.get("sourceId") ?? "SIG-4",
      }
    : null;
  const captureCriticalPhase = query.get("critical");
  const captureCriticalRequest = captureCriticalPhase
    ? { phase: captureCriticalPhase, instanceId: "capture-critical" }
    : null;
  const captureHoldProgress = Math.min(1, Math.max(0, Number(query.get("hold") ?? 0)));
  const [history, setHistory] = useState(["home"]);
  const [activeEvent, setActiveEvent] = useState(null);
  const [eventStatus, setEventStatus] = useState("Выберите уровень для демонстрации.");
  const [eventRunCount, setEventRunCount] = useState(0);
  const eventSequence = useRef(0);
  const [sig4Config, setSig4Config] = useState(defaultSig4Config);
  const [pumpConfig, setPumpConfig] = useState(defaultPumpConfig);
  const [activeCommand, setActiveCommand] = useState(null);
  const [commandOutcome, setCommandOutcome] = useState("success");
  const commandSequence = useRef(0);
  const [activeCriticalRequest, setActiveCriticalRequest] = useState(null);
  const [criticalOutcome, setCriticalOutcome] = useState("waiting");
  const criticalSequence = useRef(0);
  const screen = history[history.length - 1];

  useEffect(() => {
    if (activeEvent?.severity !== "ordinary") {
      return undefined;
    }

    const timer = window.setTimeout(() => {
      setActiveEvent(null);
      setEventStatus("Обычное событие исчезло; запись осталась в журнале.");
    }, 4500);

    return () => window.clearTimeout(timer);
  }, [activeEvent]);

  useEffect(() => {
    if (!activeCommand) {
      return undefined;
    }

    const delays = {
      sending: 650,
      accepted: 550,
      executing: 800,
    };

    if (!(activeCommand.phase in delays)) {
      return undefined;
    }

    const timer = window.setTimeout(() => {
      if (activeCommand.phase === "sending") {
        setActiveCommand((current) => (current ? { ...current, phase: "accepted" } : null));
        return;
      }

      if (activeCommand.phase === "accepted") {
        setActiveCommand((current) => (current ? { ...current, phase: "executing" } : null));
        return;
      }

      if (activeCommand.phase === "executing") {
        if (commandOutcome === "success") {
          setSig4Config((current) => {
            if (activeCommand.kind === "threshold") {
              return { ...current, threshold: activeCommand.value };
            }
            if (activeCommand.kind === "band") {
              return { ...current, band: activeCommand.value };
            }
            if (activeCommand.kind === "observation") {
              return { ...current, observing: activeCommand.value };
            }
            return current;
          });
          if (activeCommand.kind === "pumpMode") {
            setPumpConfig((current) => ({ ...current, mode: activeCommand.value }));
          }
        }

        setActiveCommand((current) =>
          current ? { ...current, phase: commandOutcome } : null,
        );
        return;
      }

    }, delays[activeCommand.phase]);

    return () => window.clearTimeout(timer);
  }, [activeCommand, commandOutcome]);

  useEffect(() => {
    if (activeCommand?.phase !== "success") {
      return undefined;
    }

    const timer = window.setTimeout(() => setActiveCommand(null), 1000);
    return () => window.clearTimeout(timer);
  }, [activeCommand?.phase, activeCommand?.instanceId]);

  useEffect(() => {
    if (!activeCriticalRequest) {
      return undefined;
    }

    if (activeCriticalRequest.phase === "sending") {
      const timer = window.setTimeout(() => {
        setActiveCriticalRequest((current) =>
          current ? { ...current, phase: criticalOutcome } : null,
        );
      }, 700);
      return () => window.clearTimeout(timer);
    }

    if (
      activeCriticalRequest.phase === "waiting" &&
      criticalOutcome !== "waiting"
    ) {
      const timer = window.setTimeout(() => {
        setActiveCriticalRequest((current) =>
          current ? { ...current, phase: criticalOutcome } : null,
        );
      }, 400);
      return () => window.clearTimeout(timer);
    }

    return undefined;
  }, [activeCriticalRequest, criticalOutcome]);

  const navigate = (target) => {
    setHistory((current) => [...current, target]);
  };

  const replaceCurrent = (target) => {
    setHistory((current) => [...current.slice(0, -1), target]);
  };

  const goBack = () => {
    if (activeCommand || activeCriticalRequest) {
      return;
    }

    if (activeEvent?.severity === "blocking") {
      return;
    }

    if (activeEvent?.severity === "important") {
      setActiveEvent(null);
      setEventStatus("Важное событие отложено и осталось непрочитанным.");
      return;
    }

    setHistory((current) => (current.length > 1 ? current.slice(0, -1) : current));
  };

  const resetNavigation = () => {
    setHistory(["home"]);
    setActiveEvent(null);
    setActiveCommand(null);
    setActiveCriticalRequest(null);
  };

  const startCommand = (command) => {
    commandSequence.current += 1;
    setActiveCommand({
      ...command,
      sourceId: command.sourceId ?? "SIG-4",
      transport: command.transport ?? "LoRa",
      phase: "sending",
      instanceId: commandSequence.current,
    });
  };

  const startCriticalRequest = () => {
    criticalSequence.current += 1;
    setActiveCriticalRequest({
      phase: "sending",
      sourceId: "PUMP-2",
      capability: "pump.stop",
      risk: "critical",
      instanceId: criticalSequence.current,
    });
  };

  const retryCommand = () => {
    commandSequence.current += 1;
    setActiveCommand((current) =>
      current
        ? {
            ...current,
            phase: "sending",
            instanceId: commandSequence.current,
          }
        : null,
    );
  };

  const triggerEvent = (event) => {
    eventSequence.current += 1;
    const run = eventSequence.current;
    setEventRunCount(run);

    if (event.severity === "background") {
      setActiveEvent(null);
      setEventStatus(`Фоновое событие записано без прерывания интерфейса · запуск ${run}.`);
      return;
    }

    setActiveEvent({ ...event, presentationId: run });
    setEventStatus(
      event.severity === "ordinary"
        ? `Плашка показана; исчезнет через 4,5 секунды · запуск ${run}.`
        : event.severity === "important"
          ? `Можно отложить или открыть устройство · запуск ${run}.`
          : `Навигация заблокирована до квитирования · запуск ${run}.`,
    );
  };

  if (capture) {
    return (
      <main className="capture-page">
        <WatchRenderer
          shape={shape}
          mode={mode}
          liveTime={false}
          screen={captureScreen}
          activeEvent={captureEvent}
          activeCommand={captureCommand}
          activeCriticalRequest={captureCriticalRequest}
          sig4Config={defaultSig4Config}
          pumpConfig={defaultPumpConfig}
          holdProgress={captureHoldProgress}
          onNavigate={() => {}}
          onReplace={() => {}}
          onBack={() => {}}
          onCommand={() => {}}
          onRetryCommand={() => {}}
          onDismissCommand={() => {}}
          onCriticalRequest={() => {}}
          onCancelCriticalRequest={() => {}}
          onDismissCriticalRequest={() => {}}
          onDismissEvent={() => {}}
          onAcknowledgeEvent={() => {}}
        />
      </main>
    );
  }

  return (
    <main className="prototype-shell">
      <header className="prototype-header">
        <div>
          <span className="prototype-kicker">SKIN 01 · 256×256</span>
          <h1>Строгий контекст</h1>
          <p>Часы сначала показывают время, затем состояние и только потом предлагают действие.</p>
        </div>
        <div className="prototype-status">
          <span className="status-dot" />
          RENDERER READY
        </div>
      </header>

      <section className="workbench">
        <div className="watch-stage">
          <div className={`watch-hardware watch-hardware--${shape}`}>
            <WatchRenderer
              shape={shape}
              mode={mode}
              liveTime={liveTime}
              screen={screen}
              activeEvent={activeEvent}
              activeCommand={activeCommand}
              activeCriticalRequest={activeCriticalRequest}
              sig4Config={sig4Config}
              pumpConfig={pumpConfig}
              holdProgress={0}
              onNavigate={navigate}
              onReplace={replaceCurrent}
              onBack={goBack}
              onCommand={startCommand}
              onRetryCommand={retryCommand}
              onDismissCommand={() => setActiveCommand(null)}
              onCriticalRequest={startCriticalRequest}
              onCancelCriticalRequest={() => setActiveCriticalRequest(null)}
              onDismissCriticalRequest={() => setActiveCriticalRequest(null)}
              onDismissEvent={() => {
                setActiveEvent(null);
                setEventStatus("Событие отложено; текущий экран восстановлен.");
              }}
              onAcknowledgeEvent={() => {
                setActiveEvent(null);
                setEventStatus("Событие квитировано; причина ещё может оставаться активной.");
              }}
            />
            <span className="crown" aria-hidden="true" />
            <button
              className="back-button"
              type="button"
              onClick={goBack}
              disabled={
                Boolean(activeCommand) ||
                Boolean(activeCriticalRequest) ||
                activeEvent?.severity === "blocking" ||
                (history.length === 1 && activeEvent?.severity !== "important")
              }
              aria-label="Физическая кнопка назад"
            />
          </div>
          <p className="stage-hint">
            Свайп влево — устройства. Назад — стрелка, свайп вправо или боковая кнопка.
          </p>
        </div>

        <aside className="controls" aria-label="Параметры предпросмотра">
          <div className="controls-title">
            <SlidersHorizontal size={22} weight="regular" aria-hidden="true" />
            <h2>Предпросмотр</h2>
          </div>

          <Segmented
            label="Форма"
            value={shape}
            onChange={(next) => {
              setShape(next);
              resetNavigation();
            }}
            options={[
              { value: "square", label: "Квадрат" },
              { value: "round", label: "Круг" },
            ]}
          />

          <Segmented
            label="Режим"
            value={mode}
            onChange={(next) => {
              setMode(next);
              resetNavigation();
            }}
            options={[
              { value: "normal", label: "Обычный" },
              { value: "ambient", label: "Экономный" },
            ]}
          />

          <button
            type="button"
            className={`live-toggle ${liveTime ? "is-on" : ""}`}
            aria-pressed={liveTime}
            onClick={() => setLiveTime((current) => !current)}
          >
            <Radio size={20} weight={liveTime ? "fill" : "regular"} aria-hidden="true" />
            <span>
              <strong>Живое время</strong>
              <small>{liveTime ? "синхронизировано" : "тестовое 20:42"}</small>
            </span>
          </button>

          <EventSimulator
            activeEventId={activeEvent?.id}
            runCount={eventRunCount}
            status={eventStatus}
            onTrigger={triggerEvent}
          />

          <Segmented
            label="Ответ на команду"
            value={commandOutcome}
            onChange={setCommandOutcome}
            options={commandOutcomeOptions}
          />

          <Segmented
            label="Внешнее разрешение"
            value={criticalOutcome}
            onChange={setCriticalOutcome}
            options={criticalOutcomeOptions}
          />

          <dl className="skin-facts">
            <div>
              <dt>Палитра</dt>
              <dd>1 bit</dd>
            </div>
            <div>
              <dt>Сетка</dt>
              <dd>4 px</dd>
            </div>
            <div>
              <dt>Минимальный штрих</dt>
              <dd>2 px</dd>
            </div>
            <div>
              <dt>Обновление</dt>
              <dd>по областям</dd>
            </div>
            <div>
              <dt>Контекстный путь</dt>
              <dd>4 уровня</dd>
            </div>
          </dl>
        </aside>
      </section>
    </main>
  );
}
