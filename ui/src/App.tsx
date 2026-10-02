import { onCleanup, onMount } from 'solid-js';

type TierVisualConfig = {
    tierText: string;
    letterColor: [number, number, number, number];
    tierEmptyColor: [number, number, number, number];
    textColor: [number, number, number, number];
    numberColor: [number, number, number, number];
    backgroundColor: [number, number, number, number];
    textFontWeight: number;
    numberFontWeight: number;
    textAllCaps: boolean;
    numberAllCaps: boolean;
};

type UiConfig = {
    enabled: boolean;
    showFloatingMessages: boolean;
    showComboHits: boolean;
    showComboNumber: boolean;
    showTotalComboPoints: boolean;
    editMode: boolean;
    editPreviewTier: number;
    progressDisplayMode: number;
    showTierName: boolean;
    positionXPercent: number;
    positionYPercent: number;
    scalePercent: number;
    progressBarWidth: number;
    progressBarHeight: number;
    comboLabelYOffset: number;
    comboNumberYOffset: number;
    tierYOffset: number;
    tierTextYOffset: number;
    progressBarYOffset: number;
    comboHitsYOffset: number;
    backgroundScalePercent: number;
    comboLabelScalePercent: number;
    comboNumberScalePercent: number;
    tierScalePercent: number;
    tierTextScalePercent: number;
    progressBarScalePercent: number;
    comboHitsScalePercent: number;
    notificationPositiveColor: [number, number, number, number];
    notificationNegativeColor: [number, number, number, number];
    showNotificationValue: boolean;
    notificationTexts: Record<string, string>;
    tiers: TierVisualConfig[];
};

type UiBridge = {
    updateComboMeter: (payload: string) => void;
    showComboMessage: (payload: string) => void;
    setComboTimerPaused: (payload: string) => void;
    updateComboUiSettings: (payload: string) => void;
};

type AssetEntry = {
    path: string;
    image: HTMLImageElement | null;
    failed: boolean;
};

type AssetRequest = {
    image: HTMLImageElement | null;
    path: string;
    failed: boolean;
};

const ranks = ['f', 'e', 'd', 'c', 'b', 'a', 's', 'ss', 'sss', 'z'];
const rankNames = ['Fierce', 'Exalted', 'Dauntless', 'Champion', 'Brilliant', 'Ascendant', 'Sovereign!', 'Supreme Skill!', 'Stellar S Style!', 'Zusk!'];
const defaultNotificationTexts: Record<string, string> = {
    'Hit': 'Hit',
    'Hit Taken': 'Hit Taken',
    'Dodge': 'Dodge',
    'Perfect Dodge': 'Perfect Dodge',
    'Dodged': 'Dodged',
    'Perfect Dodged': 'Perfect Dodged',
    'Parry': 'Parry',
    'Perfect Parry': 'Perfect Parry',
    'Parried': 'Parried',
    'Perfect Parried': 'Perfect Parried',
    'Undodgeable': 'Undodgeable',
    'Undodgeable Hit': 'Undodgeable Hit',
    'Unblockable': 'Unblockable',
    'Unblockable Hit': 'Unblockable Hit',
    'Stagger': 'Stagger',
};
const supportedImageExtensions = ['png', 'webp', 'jpg', 'jpeg', 'bmp', 'gif', 'svg'];
const defaultTierColors: [number, number, number, number][] = [
    [1, 1, 1, 1],
    [1, 1, 1, 1],
    [1, 1, 1, 1],
    [1, 1, 1, 1],
    [1, 1, 1, 1],
    [1, 1, 1, 1],
    [1, 1, 1, 1],
    [1, 1, 1, 1],
    [1, 1, 1, 1],
    [1, 1, 1, 1],
];
const defaultTierEmptyColor: [number, number, number, number] = [46 / 255, 51 / 255, 61 / 255, 166 / 255];
const defaultNumberColor: [number, number, number, number] = [229 / 255, 229 / 255, 229 / 255, 1];
const defaultBackgroundColor: [number, number, number, number] = [5 / 255, 5 / 255, 5 / 255, 140 / 255];
const hudFontFamily = 'ComboSystemDecorative, Cinzel Decorative, serif';

const defaultConfig = (): UiConfig => ({
    enabled: true,
    showFloatingMessages: true,
    showComboHits: true,
    showComboNumber: true,
    showTotalComboPoints: false,
    editMode: false,
    editPreviewTier: 8,
    progressDisplayMode: 3,
    showTierName: false,
    positionXPercent: 100,
    positionYPercent: 11,
    scalePercent: 159,
    progressBarWidth: 89,
    progressBarHeight: 8,
    comboLabelYOffset: -7,
    comboNumberYOffset: -7,
    tierYOffset: -10,
    tierTextYOffset: 33,
    progressBarYOffset: -42,
    comboHitsYOffset: -23,
    backgroundScalePercent: 106,
    comboLabelScalePercent: 64,
    comboNumberScalePercent: 74,
    tierScalePercent: 41,
    tierTextScalePercent: 71,
    progressBarScalePercent: 134,
    comboHitsScalePercent: 110,
    notificationPositiveColor: [0.97, 0.93, 0.54, 1],
    notificationNegativeColor: [1, 0.62, 0.62, 1],
    showNotificationValue: true,
    notificationTexts: { ...defaultNotificationTexts },
    tiers: defaultTierColors.map((letterColor, index) => ({
        tierText: rankNames[index],
        letterColor,
        tierEmptyColor: defaultTierEmptyColor,
        textColor: letterColor,
        numberColor: defaultNumberColor,
        backgroundColor: defaultBackgroundColor,
        textFontWeight: 2,
        numberFontWeight: 2,
        textAllCaps: false,
        numberAllCaps: false,
    })),
});

let bridge: UiBridge | null = null;
let pendingComboPayload: string | null = null;
let pendingSettingsPayload: string | null = null;
const pendingMessages: string[] = [];

(window as any).updateComboMeter = (payload: string) => {
    if (bridge) bridge.updateComboMeter(payload);
    else pendingComboPayload = payload;
};

(window as any).showComboMessage = (payload: string) => {
    if (bridge) bridge.showComboMessage(payload);
    else if (pendingMessages.length < 8) pendingMessages.push(payload);
};

(window as any).setComboTimerPaused = (payload: string) => {
    if (bridge) bridge.setComboTimerPaused(payload);
};

(window as any).updateComboUiSettings = (payload: string) => {
    if (bridge) bridge.updateComboUiSettings(payload);
    else pendingSettingsPayload = payload;
};

const clamp = (value: number, min: number, max: number) => Math.max(min, Math.min(max, value));

const colorToCss = (color: [number, number, number, number]) => {
    const r = Math.round(clamp(color[0], 0, 1) * 255);
    const g = Math.round(clamp(color[1], 0, 1) * 255);
    const b = Math.round(clamp(color[2], 0, 1) * 255);
    const a = clamp(color[3], 0, 1);
    return `rgba(${r},${g},${b},${a})`;
};

const normalizeImageSrc = (path: string) => {
    const trimmed = path.trim();
    if (!trimmed) return '';
    if (/^(https?:|file:|data:)/i.test(trimmed)) return trimmed;

    const normalized = trimmed.replace(/\\/g, '/');
    if (/^[a-z]:\//i.test(normalized)) {
        return `file:///${normalized}`;
    }
    return normalized;
};

function App() {
    let containerRef: HTMLDivElement | undefined;

    onMount(() => {
        if (!containerRef) return;

        let config = defaultConfig();
        let isTimerPaused = false;
        let animationFrameId = 0;
        let lastFrameTime = performance.now();
        let actualHitValue = 0;
        let actualTier = 0;
        let actualPoints = 0;
        let actualPointsRequired = 100;
        let actualTotalPoints = 0;
        const hasActiveCombo = () => actualTier > 0 || actualPoints > 0;
        let renderedTier = 0;
        let renderedProgress = 0;
        let targetProgress = 0;
        let styleDirty = true;
        let styledTier = -1;
        let lastComboNumberText = '';

        const assetCache = new Map<string, AssetEntry>();
        const tierFallbackFailed = new Set<string>();

        const hudRoot = document.createElement('div');
        const backgroundLayer = document.createElement('div');
        const comboLabelLayer = document.createElement('div');
        const comboNumberLayer = document.createElement('div');
        const tierTextLayer = document.createElement('div');
        const tierLayer = document.createElement('div');
        const tierBaseImage = document.createElement('img');
        const tierFillMask = document.createElement('div');
        const tierFillImage = document.createElement('img');
        const tierFallbackText = document.createElement('div');
        const tierFallbackFillMask = document.createElement('div');
        const tierFallbackFillText = document.createElement('div');
        const barLayer = document.createElement('div');
        const barTrack = document.createElement('div');
        const barFill = document.createElement('div');
        const barFrame = document.createElement('img');
        const hitsLayer = document.createElement('div');
        const messageLayer = document.createElement('div');

        const allLayers = [backgroundLayer, tierTextLayer, comboLabelLayer, comboNumberLayer, tierLayer, barLayer, hitsLayer, messageLayer];
        hudRoot.style.position = 'absolute';
        hudRoot.style.left = '0';
        hudRoot.style.top = '0';
        hudRoot.style.width = '320px';
        hudRoot.style.height = '245px';
        hudRoot.style.overflow = 'visible';
        hudRoot.style.pointerEvents = 'none';
        hudRoot.style.display = 'none';
        hudRoot.style.transformOrigin = 'top left';

        allLayers.forEach((layer) => {
            layer.style.position = 'absolute';
            layer.style.pointerEvents = 'none';
            layer.style.overflow = 'visible';
        });

        backgroundLayer.style.zIndex = '10';
        backgroundLayer.style.left = '50%';
        backgroundLayer.style.top = '-44px';
        backgroundLayer.style.width = '368px';
        backgroundLayer.style.height = '210px';
        backgroundLayer.style.backgroundRepeat = 'no-repeat';
        backgroundLayer.style.backgroundPosition = 'center';
        backgroundLayer.style.backgroundSize = 'contain';
        backgroundLayer.style.transform = 'translateX(-50%)';

        tierTextLayer.style.zIndex = '20';
        tierTextLayer.style.left = '50%';
        tierTextLayer.style.top = '126px';
        tierTextLayer.style.width = '352px';
        tierTextLayer.style.height = '54px';
        tierTextLayer.style.backgroundRepeat = 'no-repeat';
        tierTextLayer.style.backgroundPosition = 'center';
        tierTextLayer.style.backgroundSize = 'contain';
        tierTextLayer.style.textAlign = 'center';
        tierTextLayer.style.font = `bold 30px ${hudFontFamily}`;
        tierTextLayer.style.lineHeight = '54px';
        tierTextLayer.style.transform = 'translateX(-50%)';

        comboLabelLayer.style.zIndex = '30';
        comboLabelLayer.style.left = '50%';
        comboLabelLayer.style.top = '-4px';
        comboLabelLayer.style.width = '200px';
        comboLabelLayer.style.height = '38px';
        comboLabelLayer.style.backgroundRepeat = 'no-repeat';
        comboLabelLayer.style.backgroundPosition = 'center';
        comboLabelLayer.style.backgroundSize = 'contain';
        comboLabelLayer.style.textAlign = 'center';
        comboLabelLayer.style.font = `bold 28px ${hudFontFamily}`;
        comboLabelLayer.style.lineHeight = '38px';
        comboLabelLayer.style.transform = 'translateX(-50%)';

        comboNumberLayer.style.zIndex = '40';
        comboNumberLayer.style.left = '50%';
        comboNumberLayer.style.top = '20px';
        comboNumberLayer.style.width = '344px';
        comboNumberLayer.style.height = '92px';
        comboNumberLayer.style.textAlign = 'center';
        comboNumberLayer.style.font = `900 86px ${hudFontFamily}`;
        comboNumberLayer.style.lineHeight = '92px';
        comboNumberLayer.style.letterSpacing = '0';
        comboNumberLayer.style.justifyContent = 'center';
        comboNumberLayer.style.alignItems = 'center';
        comboNumberLayer.style.transform = 'translateX(-50%)';

        tierLayer.style.zIndex = '50';
        tierLayer.style.left = '50%';
        tierLayer.style.top = '72px';
        tierLayer.style.width = '384px';
        tierLayer.style.height = '96px';
        tierLayer.style.overflow = 'visible';
        tierLayer.style.transform = 'translateX(-50%)';

        [tierBaseImage, tierFillImage].forEach((image) => {
            image.style.position = 'absolute';
            image.style.left = '0';
            image.style.width = '100%';
            image.style.height = '96px';
            image.style.objectFit = 'contain';
            image.style.pointerEvents = 'none';
        });
        tierBaseImage.style.top = '0';
        tierFillImage.style.bottom = '0';
        tierBaseImage.style.height = '100%';
        tierBaseImage.style.opacity = '0.5';
        tierFillMask.style.position = 'absolute';
        tierFillMask.style.left = '0';
        tierFillMask.style.bottom = '0';
        tierFillMask.style.width = '100%';
        tierFillMask.style.height = '0%';
        tierFillMask.style.overflow = 'hidden';
        tierFillMask.style.pointerEvents = 'none';
        tierFallbackText.style.position = 'absolute';
        tierFallbackText.style.left = '0';
        tierFallbackText.style.top = '0';
        tierFallbackText.style.width = '100%';
        tierFallbackText.style.height = '100%';
        tierFallbackText.style.textAlign = 'center';
        tierFallbackText.style.font = `900 92px ${hudFontFamily}`;
        tierFallbackText.style.lineHeight = '96px';
        tierFallbackText.style.letterSpacing = '0';
        tierFallbackText.style.display = 'none';
        tierFallbackFillMask.style.position = 'absolute';
        tierFallbackFillMask.style.left = '0';
        tierFallbackFillMask.style.bottom = '0';
        tierFallbackFillMask.style.width = '100%';
        tierFallbackFillMask.style.height = '0%';
        tierFallbackFillMask.style.overflow = 'hidden';
        tierFallbackFillMask.style.pointerEvents = 'none';
        tierFallbackFillMask.style.display = 'none';
        tierFallbackFillText.style.position = 'absolute';
        tierFallbackFillText.style.left = '0';
        tierFallbackFillText.style.bottom = '0';
        tierFallbackFillText.style.width = '100%';
        tierFallbackFillText.style.height = '96px';
        tierFallbackFillText.style.textAlign = 'center';
        tierFallbackFillText.style.font = `900 92px ${hudFontFamily}`;
        tierFallbackFillText.style.lineHeight = '96px';
        tierFallbackFillText.style.letterSpacing = '0';
        tierFillMask.appendChild(tierFillImage);
        tierFallbackFillMask.appendChild(tierFallbackFillText);
        tierLayer.appendChild(tierBaseImage);
        tierLayer.appendChild(tierFillMask);
        tierLayer.appendChild(tierFallbackText);
        tierLayer.appendChild(tierFallbackFillMask);

        barLayer.style.zIndex = '60';
        barLayer.style.left = '50%';
        barLayer.style.top = '184px';
        barLayer.style.width = '220px';
        barLayer.style.height = '13px';
        barLayer.style.overflow = 'visible';
        barLayer.style.transform = 'translateX(-50%)';
        barTrack.style.position = 'absolute';
        barTrack.style.left = '0';
        barTrack.style.top = '0';
        barTrack.style.width = '100%';
        barTrack.style.height = '100%';
        barTrack.style.borderRadius = '4px';
        barTrack.style.overflow = 'hidden';
        barFill.style.position = 'absolute';
        barFill.style.left = '3px';
        barFill.style.top = '3px';
        barFill.style.width = '0px';
        barFill.style.height = '7px';
        barFill.style.borderRadius = '3px';
        barFrame.style.position = 'absolute';
        barFrame.style.left = '0';
        barFrame.style.top = '0';
        barFrame.style.width = '100%';
        barFrame.style.height = '100%';
        barFrame.style.objectFit = 'fill';
        barFrame.style.display = 'none';
        barFrame.style.pointerEvents = 'none';
        barLayer.appendChild(barTrack);
        barLayer.appendChild(barFill);
        barLayer.appendChild(barFrame);

        hitsLayer.style.zIndex = '70';
        hitsLayer.style.left = '50%';
        hitsLayer.style.top = '204px';
        hitsLayer.style.width = '220px';
        hitsLayer.style.height = '24px';
        hitsLayer.style.textAlign = 'center';
        hitsLayer.style.font = `bold 16px ${hudFontFamily}`;
        hitsLayer.style.lineHeight = '24px';
        hitsLayer.style.letterSpacing = '1px';
        hitsLayer.style.transform = 'translateX(-50%)';

        messageLayer.style.zIndex = '80';
        messageLayer.style.left = '0';
        messageLayer.style.top = '0';
        messageLayer.style.width = '320px';
        messageLayer.style.height = '120px';

        hudRoot.appendChild(backgroundLayer);
        hudRoot.appendChild(tierTextLayer);
        hudRoot.appendChild(comboLabelLayer);
        hudRoot.appendChild(comboNumberLayer);
        hudRoot.appendChild(tierLayer);
        hudRoot.appendChild(barLayer);
        hudRoot.appendChild(hitsLayer);
        hudRoot.appendChild(messageLayer);
        containerRef.appendChild(hudRoot);

        type FloatingMessage = {
            node: HTMLDivElement;
            active: boolean;
            start: number;
            duration: number;
            startX: number;
            startY: number;
        };

        const messages: FloatingMessage[] = [];
        for (let i = 0; i < 10; i++) {
            const node = document.createElement('div');
            node.style.position = 'absolute';
            node.style.width = '170px';
            node.style.textAlign = 'center';
            node.style.font = `bold 17px ${hudFontFamily}`;
            node.style.display = 'none';
            node.style.pointerEvents = 'none';
            messageLayer.appendChild(node);
            messages.push({ node, active: false, start: 0, duration: 820, startX: 0, startY: 0 });
        }

        const getTierConfig = (tier: number) => config.tiers[clamp(tier, 0, ranks.length - 1)] ?? defaultConfig().tiers[0];
        const getUiScale = () => clamp(config.scalePercent, 40, 300) / 100;
        const getBarWidth = () => clamp(config.progressBarWidth, 60, 600);
        const getBarHeight = () => clamp(config.progressBarHeight, 6, 60);
        const getBarFillWidth = () => Math.max(1, getBarWidth() - 6);
        const showsBarProgress = () => config.progressDisplayMode === 1 || config.progressDisplayMode === 3;
        const showsTierProgress = () => config.progressDisplayMode === 2 || config.progressDisplayMode === 3;
        const getLayerScale = (percent: number) => clamp(percent, 25, 300) / 100;
        const getFontWeight = (value: number) => {
            const normalized = Math.round(clamp(value, 0, 2));
            if (normalized === 0) return 300;
            if (normalized === 1) return 500;
            return 700;
        };
        const applyCaps = (text: string, enabled: boolean) => enabled ? text.toUpperCase() : text;

        const getTierName = (tier: number) => ranks[clamp(tier, 0, ranks.length - 1)];
        const tierAssetBasePath = (folder: string, tier: number) => `./Assets/ComboSystem/${folder}/${getTierName(tier)}`;
        const tierFolderAssetBasePath = (tier: number, assetName: string) => `./Assets/ComboSystem/${getTierName(tier)}/${assetName}`;
        const defaultAssetBasePath = (assetName: string) => `./Assets/ComboSystem/Default/${assetName}`;
        const legacyGenericAssetBasePath = (folder: string, name: string) => `./Assets/ComboSystem/${folder}/${name}`;
        const legacyFontAssetBasePath = (character: string) => `./Assets/ComboSystem/FontAssets/${character}`;
        const cacheKey = (basePath: string, extensions = supportedImageExtensions) => `${basePath}|${extensions.join(',')}`;
        const tierAssetBasePaths = (tier: number, assetName: string, legacyFolder: string) => [
            tierFolderAssetBasePath(tier, assetName),
            defaultAssetBasePath(assetName),
            tierAssetBasePath(legacyFolder, tier),
        ];
        const comboLabelBasePaths = (tier: number) => [
            tierFolderAssetBasePath(tier, 'ComboLabel'),
            defaultAssetBasePath('ComboLabel'),
            tierAssetBasePath('ComboLabel', tier),
            legacyGenericAssetBasePath('ComboLabel', 'Default'),
        ];
        const fontAssetBasePaths = (tier: number, character: string) => [
            tierFolderAssetBasePath(tier, `FontAssets/${character}`),
            defaultAssetBasePath(`FontAssets/${character}`),
            legacyFontAssetBasePath(character),
        ];

        const requestAsset = (
            basePath: string,
            onChange: () => void,
            extensions = supportedImageExtensions
        ) => {
            const key = cacheKey(basePath, extensions);
            const cached = assetCache.get(key);
            if (cached) return cached.image;

            const candidates = extensions.map((extension) => `${basePath}.${extension}`);
            const entry: AssetEntry = { path: candidates[0], image: null, failed: false };
            assetCache.set(key, entry);

            let index = 0;
            const tryNext = () => {
                if (index >= candidates.length) {
                    entry.failed = true;
                    onChange();
                    return;
                }
                const path = candidates[index];
                const image = new Image();
                entry.path = path;
                image.onload = () => {
                    entry.image = image;
                    entry.failed = false;
                    onChange();
                };
                image.onerror = () => {
                    index += 1;
                    tryNext();
                };
                image.src = normalizeImageSrc(path);
            };
            tryNext();
            return null;
        };

        const assetPreloadChanged = () => {
            if (hasActiveCombo() || config.editMode) {
                styleDirty = true;
                applyTierStyle();
                renderComboNumber();
            }
        };

        const getAssetPath = (basePath: string, extensions = supportedImageExtensions) =>
            assetCache.get(cacheKey(basePath, extensions))?.path ?? '';
        const hasAssetFailed = (basePath: string, extensions = supportedImageExtensions) =>
            Boolean(assetCache.get(cacheKey(basePath, extensions))?.failed);

        const requestAssetFromBases = (
            basePaths: string[],
            onChange: () => void,
            extensions = supportedImageExtensions
        ): AssetRequest => {
            for (const basePath of basePaths) {
                const image = requestAsset(basePath, onChange, extensions);
                if (image) {
                    return { image, path: getAssetPath(basePath, extensions), failed: false };
                }
                if (!hasAssetFailed(basePath, extensions)) {
                    return { image: null, path: '', failed: false };
                }
            }
            return { image: null, path: '', failed: true };
        };

        const tierScopedAsset = (tier: number, assetName: string, legacyFolder: string) =>
            requestAssetFromBases(tierAssetBasePaths(tier, assetName, legacyFolder), markStyleDirty);

        const tierScopedFontAsset = (tier: number, character: string) =>
            requestAssetFromBases(fontAssetBasePaths(tier, character), renderComboNumber);

        const applySolidTextColor = (node: HTMLElement, color: string) => {
            node.style.color = color;
            node.style.backgroundImage = '';
            node.style.webkitBackgroundClip = '';
            node.style.backgroundClip = '';
        };

        const applyAssetBackground = (node: HTMLElement, image: HTMLImageElement | null) => {
            node.style.backgroundImage = image ? `url("${image.src}")` : '';
        };

        const getFontAssetName = (character: string) => {
            if (/^[0-9]$/.test(character)) return character;
            if (character === '-') return 'minus';
            if (character === '+') return 'plus';
            return '';
        };

        const renderComboNumber = () => {
            const text = lastComboNumberText;
            if (!config.showComboNumber || !text) {
                comboNumberLayer.style.display = 'none';
                comboNumberLayer.textContent = '';
                return;
            }

            const tier = clamp(renderedTier, 0, ranks.length - 1);
            const tierConfig = getTierConfig(tier);
            const color = colorToCss(tierConfig.numberColor);
            const assets: HTMLImageElement[] = [];
            let canUseAssets = true;

            for (const character of text) {
                const assetName = getFontAssetName(character);
                if (!assetName) {
                    canUseAssets = false;
                    break;
                }

                const asset = tierScopedFontAsset(tier, assetName);
                if (!asset.image || asset.failed) {
                    canUseAssets = false;
                    break;
                }
                assets.push(asset.image);
            }

            comboNumberLayer.style.display = 'flex';
            comboNumberLayer.textContent = '';

            if (!canUseAssets) {
                comboNumberLayer.style.display = 'block';
                comboNumberLayer.textContent = applyCaps(text, tierConfig.numberAllCaps);
                comboNumberLayer.style.fontWeight = `${getFontWeight(tierConfig.numberFontWeight)}`;
                applySolidTextColor(comboNumberLayer, color);
                return;
            }

            comboNumberLayer.style.color = '';
            comboNumberLayer.style.backgroundImage = '';
            comboNumberLayer.style.webkitBackgroundClip = '';
            comboNumberLayer.style.backgroundClip = '';
            const fragment = document.createDocumentFragment();
            for (const asset of assets) {
                const image = document.createElement('img');
                image.src = asset.src;
                image.style.display = 'block';
                image.style.height = '92px';
                image.style.width = 'auto';
                image.style.marginLeft = '-2px';
                image.style.marginRight = '-2px';
                image.style.objectFit = 'contain';
                image.style.pointerEvents = 'none';
                fragment.appendChild(image);
            }
            comboNumberLayer.appendChild(fragment);
        };

        const positionHud = () => {
            const scale = getUiScale();
            const hudWidth = Math.max(320, getBarWidth() + 100);
            const hudHeight = Math.max(245, 218 + getBarHeight());
            const rawX = window.innerWidth * (clamp(config.positionXPercent, 0, 100) / 100);
            const rawY = window.innerHeight * (clamp(config.positionYPercent, 0, 100) / 100);
            const anchorX = clamp(config.positionXPercent, 0, 100) / 100;
            const anchorY = clamp(config.positionYPercent, 0, 100) / 100;
            const x = clamp(rawX - ((hudWidth * scale) * anchorX), 0, Math.max(0, window.innerWidth - (hudWidth * scale)));
            const y = clamp(rawY - ((hudHeight * scale) * anchorY), 0, Math.max(0, window.innerHeight - (hudHeight * scale)));

            hudRoot.style.left = `${x}px`;
            hudRoot.style.top = `${y}px`;
            hudRoot.style.width = `${hudWidth}px`;
            hudRoot.style.height = `${hudHeight}px`;
            hudRoot.style.transform = `scale(${scale})`;

            comboLabelLayer.style.top = `${-4 + config.comboLabelYOffset}px`;
            comboNumberLayer.style.top = `${20 + config.comboNumberYOffset}px`;
            tierLayer.style.top = `${72 + config.tierYOffset}px`;
            tierTextLayer.style.top = `${126 + config.tierTextYOffset}px`;
            barLayer.style.top = `${184 + config.progressBarYOffset}px`;
            backgroundLayer.style.width = `${hudWidth + 48}px`;
            backgroundLayer.style.transform = `translateX(-50%) scale(${getLayerScale(config.backgroundScalePercent)})`;
            comboLabelLayer.style.transform = `translateX(-50%) scale(${getLayerScale(config.comboLabelScalePercent)})`;
            comboNumberLayer.style.transform = `translateX(-50%) scale(${getLayerScale(config.comboNumberScalePercent)})`;
            tierLayer.style.transform = `translateX(-50%) scale(${getLayerScale(config.tierScalePercent)})`;
            tierTextLayer.style.transform = `translateX(-50%) scale(${getLayerScale(config.tierTextScalePercent)})`;
            barLayer.style.transform = `translateX(-50%) scale(${getLayerScale(config.progressBarScalePercent)})`;
            hitsLayer.style.transform = `translateX(-50%) scale(${getLayerScale(config.comboHitsScalePercent)})`;
            barLayer.style.width = `${getBarWidth()}px`;
            barLayer.style.height = `${getBarHeight()}px`;
            barFill.style.height = `${Math.max(1, getBarHeight() - 6)}px`;
            barFrame.style.height = `${getBarHeight()}px`;
            hitsLayer.style.top = `${184 + getBarHeight() + 5 + config.comboHitsYOffset}px`;
            hitsLayer.style.width = `${getBarWidth()}px`;
            comboNumberLayer.style.width = `${hudWidth}px`;
            tierTextLayer.style.width = `${hudWidth + 32}px`;
            tierLayer.style.width = `${hudWidth + 64}px`;
            messageLayer.style.width = `${hudWidth}px`;
        };

        const setHudVisible = (visible: boolean) => {
            hudRoot.style.display = visible ? 'block' : 'none';
        };

        const updateProgress = () => {
            const progress = clamp(renderedProgress, 0, 1);
            const fillWidth = getBarFillWidth() * progress;
            barFill.style.width = `${fillWidth}px`;
            tierFillMask.style.height = `${progress * 100}%`;
            tierFallbackFillMask.style.height = `${progress * 100}%`;
        };

        const markStyleDirty = () => {
            styleDirty = true;
            applyTierStyle();
        };

        function resolveComboLabelAsset(tier: number) {
            const asset = requestAssetFromBases(comboLabelBasePaths(tier), markStyleDirty);
            if (asset.image) return asset.image;
            if (asset.failed) tierFallbackFailed.add('ComboLabel');
            return null;
        }

        const preloadAllAssets = () => {
            const preloadBases = (basePaths: string[]) => {
                for (const basePath of basePaths) {
                    requestAsset(basePath, assetPreloadChanged);
                }
            };

            for (let tier = 0; tier < ranks.length; tier++) {
                preloadBases(tierAssetBasePaths(tier, 'Background', 'Backgrounds'));
                preloadBases(comboLabelBasePaths(tier));
                preloadBases(tierAssetBasePaths(tier, 'Text', 'Text'));
                preloadBases(tierAssetBasePaths(tier, 'Tier', 'Tiers'));
                preloadBases(tierAssetBasePaths(tier, 'Bar', 'Bars'));
                for (let digit = 0; digit <= 9; digit++) {
                    preloadBases(fontAssetBasePaths(tier, `${digit}`));
                }
                preloadBases(fontAssetBasePaths(tier, 'minus'));
                preloadBases(fontAssetBasePaths(tier, 'plus'));
            }
        };

        function applyTierStyle() {
            const tier = clamp(renderedTier, 0, ranks.length - 1);
            const tierConfig = getTierConfig(tier);
            const color = colorToCss(tierConfig.letterColor);
            const emptyColor = colorToCss(tierConfig.tierEmptyColor);
            const textColor = colorToCss(tierConfig.textColor);
            const numberColor = colorToCss(tierConfig.numberColor);
            const backgroundColor = colorToCss(tierConfig.backgroundColor);

            const backgroundAsset = tierScopedAsset(tier, 'Background', 'Backgrounds').image;
            const comboLabelAsset = resolveComboLabelAsset(tier);
            const textRequest = config.showTierName ? tierScopedAsset(tier, 'Text', 'Text') : { image: null, path: '', failed: false };
            const tierRequest = tierScopedAsset(tier, 'Tier', 'Tiers');
            const barRequest = showsBarProgress() ? tierScopedAsset(tier, 'Bar', 'Bars') : { image: null, path: '', failed: false };
            const textAsset = textRequest.image;
            const tierAsset = tierRequest.image;
            const barAsset = barRequest.image;

            applyAssetBackground(backgroundLayer, backgroundAsset);

            if (comboLabelAsset) {
                applyAssetBackground(comboLabelLayer, comboLabelAsset);
                comboLabelLayer.textContent = '';
            } else if (tierFallbackFailed.has('ComboLabel')) {
                applyAssetBackground(comboLabelLayer, null);
                comboLabelLayer.textContent = applyCaps('combo', tierConfig.textAllCaps);
                comboLabelLayer.style.color = textColor;
                comboLabelLayer.style.fontWeight = `${getFontWeight(tierConfig.textFontWeight)}`;
            }

            if (config.showTierName) {
                tierTextLayer.style.display = 'block';
                if (textAsset) {
                    applyAssetBackground(tierTextLayer, textAsset);
                    tierTextLayer.textContent = '';
                } else if (textRequest.failed) {
                    applyAssetBackground(tierTextLayer, null);
                    tierTextLayer.textContent = applyCaps(tierConfig.tierText || rankNames[tier], tierConfig.textAllCaps);
                    tierTextLayer.style.color = textColor;
                    tierTextLayer.style.fontWeight = `${getFontWeight(tierConfig.textFontWeight)}`;
                }
            } else {
                tierTextLayer.style.display = 'none';
            }

            tierBaseImage.style.display = 'none';
            tierFillMask.style.display = 'none';
            tierFallbackText.style.display = 'none';
            tierFallbackFillMask.style.display = 'none';
            if (tierAsset) {
                tierBaseImage.src = normalizeImageSrc(tierRequest.path);
                tierFillImage.src = normalizeImageSrc(tierRequest.path);
                tierBaseImage.style.opacity = showsTierProgress() ? '0.5' : '1';
                tierBaseImage.style.display = 'block';
                tierFillMask.style.display = showsTierProgress() ? 'block' : 'none';
            } else if (tierRequest.failed) {
                const tierText = ranks[tier];
                tierFallbackText.textContent = tierText;
                tierFallbackText.style.color = showsTierProgress() ? emptyColor : color;
                tierFallbackFillText.textContent = tierText;
                tierFallbackFillText.style.color = color;
                tierFallbackText.style.display = 'block';
                tierFallbackFillMask.style.display = showsTierProgress() ? 'block' : 'none';
            }

            renderComboNumber();

            hitsLayer.style.display = config.showComboHits ? 'block' : 'none';
            hitsLayer.style.fontWeight = `${getFontWeight(tierConfig.numberFontWeight)}`;
            applySolidTextColor(hitsLayer, numberColor);

            barLayer.style.display = showsBarProgress() ? 'block' : 'none';
            barTrack.style.background = backgroundColor;
            barFill.style.background = color;
            if (barAsset) {
                barFrame.src = normalizeImageSrc(barRequest.path);
                barFrame.style.display = 'block';
            } else {
                barFrame.style.display = 'none';
            }

            styledTier = tier;
            styleDirty = false;
            positionHud();
            updateProgress();
        }

        const requestFrame = () => {
            if (animationFrameId === 0) {
                lastFrameTime = performance.now();
                animationFrameId = requestAnimationFrame(updateLoop);
            }
        };

        const hasActiveMessages = () => messages.some((message) => message.active);

        const clearCombo = () => {
            renderedTier = 0;
            renderedProgress = 0;
            targetProgress = 0;
            comboNumberLayer.textContent = '';
            lastComboNumberText = '';
            hitsLayer.textContent = '';
            updateProgress();
            setHudVisible(false);
            for (const message of messages) {
                message.active = false;
                message.node.style.display = 'none';
            }
        };

        const renderCombo = (hitValue: number, tierValue: number, comboPoints: number, pointsRequired: number, totalComboPoints = comboPoints) => {
            const nextTier = clamp(tierValue, 0, ranks.length - 1);
            const tierChanged = nextTier !== renderedTier;
            renderedTier = nextTier;
            targetProgress = clamp(comboPoints / Math.max(1, pointsRequired), 0, 1);

            lastComboNumberText = `${config.showTotalComboPoints ? totalComboPoints : comboPoints}`;
            renderComboNumber();
            const tierConfig = getTierConfig(nextTier);
            hitsLayer.textContent = applyCaps(`${hitValue} hits`, tierConfig.numberAllCaps);
            setHudVisible(true);

            if (styleDirty || tierChanged || styledTier !== renderedTier) {
                applyTierStyle();
            }
            requestFrame();
        };

        const renderEditPreview = () => {
            if (config.editMode) {
                renderCombo(327, config.editPreviewTier, 70, 100);
            } else if (hasActiveCombo()) {
                renderCombo(actualHitValue, actualTier, actualPoints, actualPointsRequired, actualTotalPoints);
            } else {
                clearCombo();
            }
        };

        function updateLoop() {
            animationFrameId = 0;
            const now = performance.now();
            const deltaTime = Math.min((now - lastFrameTime) / 1000, 0.1);
            lastFrameTime = now;
            let needsFrame = false;

            if (!isTimerPaused) {
                const easing = Math.min(1, deltaTime * 14);
                const nextProgress = renderedProgress + ((targetProgress - renderedProgress) * easing);
                const finalProgress = Math.abs(nextProgress - targetProgress) < 0.004 ? targetProgress : nextProgress;
                if (Math.abs(finalProgress - renderedProgress) > 0.001) {
                    renderedProgress = finalProgress;
                    updateProgress();
                    needsFrame = true;
                }

                for (const message of messages) {
                    if (!message.active) continue;
                    const progress = Math.min(1, (now - message.start) / message.duration);
                    const lift = 54 * progress;
                    message.node.style.transform = `translate(${message.startX}px, ${message.startY - lift}px)`;
                    message.node.style.opacity = `${1 - progress}`;
                    if (progress >= 1) {
                        message.active = false;
                        message.node.style.display = 'none';
                    } else {
                        needsFrame = true;
                    }
                }
            }

            if (needsFrame || hasActiveMessages()) {
                requestFrame();
            }
        }

        const spawnMessage = (label: string, delta: number) => {
            if (!label || delta === 0) return;
            const slot = messages.find((message) => !message.active) ?? messages[0];
            const sign = delta > 0 ? '+' : '';
            const displayLabel = config.notificationTexts[label] || label;
            const valueText = config.showNotificationValue ? ` ${sign}${delta}` : '';

            slot.active = true;
            slot.start = performance.now();
            slot.startX = 74 + (Math.random() * 72);
            slot.startY = 22 + (Math.random() * 16);
            slot.node.textContent = `${displayLabel}${valueText}`;
            slot.node.style.color = colorToCss(delta > 0 ? config.notificationPositiveColor : config.notificationNegativeColor);
            slot.node.style.opacity = '1';
            slot.node.style.display = 'block';
            slot.node.style.transform = `translate(${slot.startX}px, ${slot.startY}px)`;
            requestFrame();
        };

        const parseConfig = (payload: string) => {
            const parsed = JSON.parse(payload) as Partial<UiConfig>;
            const next = defaultConfig();
            next.enabled = parsed.enabled !== false;
            next.showFloatingMessages = parsed.showFloatingMessages !== false;
            next.showComboHits = parsed.showComboHits !== false;
            next.showComboNumber = parsed.showComboNumber !== false;
            next.showTotalComboPoints = parsed.showTotalComboPoints === true;
            next.editMode = Boolean(parsed.editMode);
            const previewTier = Number(parsed.editPreviewTier ?? next.editPreviewTier);
            next.editPreviewTier = Number.isFinite(previewTier)
                ? clamp(Math.trunc(previewTier), 0, ranks.length - 1) : next.editPreviewTier;
            if (typeof (parsed as any).progressDisplayMode === 'number') {
                next.progressDisplayMode = clamp(Number((parsed as any).progressDisplayMode), 0, 3);
            } else {
                next.progressDisplayMode = Boolean((parsed as any).useTextProgressFill) ? 2 : 1;
            }
            next.showTierName = parsed.showTierName !== false;
            next.positionXPercent = clamp(Number(parsed.positionXPercent ?? next.positionXPercent), 0, 100);
            next.positionYPercent = clamp(Number(parsed.positionYPercent ?? next.positionYPercent), 0, 100);
            next.scalePercent = clamp(Number(parsed.scalePercent ?? next.scalePercent), 40, 300);
            next.progressBarWidth = clamp(Number(parsed.progressBarWidth ?? next.progressBarWidth), 60, 600);
            next.progressBarHeight = clamp(Number(parsed.progressBarHeight ?? next.progressBarHeight), 6, 60);
            next.comboLabelYOffset = clamp(Number(parsed.comboLabelYOffset ?? next.comboLabelYOffset), -300, 300);
            next.comboNumberYOffset = clamp(Number(parsed.comboNumberYOffset ?? next.comboNumberYOffset), -300, 300);
            next.tierYOffset = clamp(Number(parsed.tierYOffset ?? next.tierYOffset), -300, 300);
            next.tierTextYOffset = clamp(Number(parsed.tierTextYOffset ?? next.tierTextYOffset), -300, 300);
            next.progressBarYOffset = clamp(Number(parsed.progressBarYOffset ?? next.progressBarYOffset), -300, 300);
            next.comboHitsYOffset = clamp(Number(parsed.comboHitsYOffset ?? next.comboHitsYOffset), -300, 300);
            next.backgroundScalePercent = clamp(Number(parsed.backgroundScalePercent ?? next.backgroundScalePercent), 25, 300);
            next.comboLabelScalePercent = clamp(Number(parsed.comboLabelScalePercent ?? next.comboLabelScalePercent), 25, 300);
            next.comboNumberScalePercent = clamp(Number(parsed.comboNumberScalePercent ?? next.comboNumberScalePercent), 25, 300);
            next.tierScalePercent = clamp(Number(parsed.tierScalePercent ?? next.tierScalePercent), 25, 300);
            next.tierTextScalePercent = clamp(Number(parsed.tierTextScalePercent ?? next.tierTextScalePercent), 25, 300);
            next.progressBarScalePercent = clamp(Number(parsed.progressBarScalePercent ?? next.progressBarScalePercent), 25, 300);
            next.comboHitsScalePercent = clamp(Number(parsed.comboHitsScalePercent ?? next.comboHitsScalePercent), 25, 300);
            if (Array.isArray(parsed.notificationPositiveColor) && parsed.notificationPositiveColor.length >= 3) {
                next.notificationPositiveColor = [
                    clamp(Number(parsed.notificationPositiveColor[0]), 0, 1),
                    clamp(Number(parsed.notificationPositiveColor[1]), 0, 1),
                    clamp(Number(parsed.notificationPositiveColor[2]), 0, 1),
                    clamp(Number(parsed.notificationPositiveColor[3] ?? 1), 0, 1),
                ];
            }
            if (Array.isArray(parsed.notificationNegativeColor) && parsed.notificationNegativeColor.length >= 3) {
                next.notificationNegativeColor = [
                    clamp(Number(parsed.notificationNegativeColor[0]), 0, 1),
                    clamp(Number(parsed.notificationNegativeColor[1]), 0, 1),
                    clamp(Number(parsed.notificationNegativeColor[2]), 0, 1),
                    clamp(Number(parsed.notificationNegativeColor[3] ?? 1), 0, 1),
                ];
            }
            next.showNotificationValue = parsed.showNotificationValue !== false;
            if (parsed.notificationTexts && typeof parsed.notificationTexts === 'object') {
                for (const [key, value] of Object.entries(parsed.notificationTexts)) {
                    if (typeof value === 'string') {
                        next.notificationTexts[key] = value;
                    }
                }
            }

            if (Array.isArray(parsed.tiers)) {
                for (let i = 0; i < Math.min(parsed.tiers.length, ranks.length); i++) {
                    const tier = parsed.tiers[i] as Partial<TierVisualConfig>;
                    if (typeof tier.tierText === 'string') {
                        next.tiers[i].tierText = tier.tierText;
                    }
                    if (Array.isArray(tier.letterColor) && tier.letterColor.length >= 3) {
                        next.tiers[i].letterColor = [
                            clamp(Number(tier.letterColor[0]), 0, 1),
                            clamp(Number(tier.letterColor[1]), 0, 1),
                            clamp(Number(tier.letterColor[2]), 0, 1),
                            clamp(Number(tier.letterColor[3] ?? 1), 0, 1),
                        ];
                    }
                    if (Array.isArray(tier.tierEmptyColor) && tier.tierEmptyColor.length >= 3) {
                        next.tiers[i].tierEmptyColor = [
                            clamp(Number(tier.tierEmptyColor[0]), 0, 1),
                            clamp(Number(tier.tierEmptyColor[1]), 0, 1),
                            clamp(Number(tier.tierEmptyColor[2]), 0, 1),
                            clamp(Number(tier.tierEmptyColor[3] ?? 1), 0, 1),
                        ];
                    }
                    if (Array.isArray(tier.textColor) && tier.textColor.length >= 3) {
                        next.tiers[i].textColor = [
                            clamp(Number(tier.textColor[0]), 0, 1),
                            clamp(Number(tier.textColor[1]), 0, 1),
                            clamp(Number(tier.textColor[2]), 0, 1),
                            clamp(Number(tier.textColor[3] ?? 1), 0, 1),
                        ];
                    }
                    if (Array.isArray(tier.numberColor) && tier.numberColor.length >= 3) {
                        next.tiers[i].numberColor = [
                            clamp(Number(tier.numberColor[0]), 0, 1),
                            clamp(Number(tier.numberColor[1]), 0, 1),
                            clamp(Number(tier.numberColor[2]), 0, 1),
                            clamp(Number(tier.numberColor[3] ?? 1), 0, 1),
                        ];
                    }
                    if (Array.isArray(tier.backgroundColor) && tier.backgroundColor.length >= 3) {
                        next.tiers[i].backgroundColor = [
                            clamp(Number(tier.backgroundColor[0]), 0, 1),
                            clamp(Number(tier.backgroundColor[1]), 0, 1),
                            clamp(Number(tier.backgroundColor[2]), 0, 1),
                            clamp(Number(tier.backgroundColor[3] ?? 1), 0, 1),
                        ];
                    }
                    next.tiers[i].textFontWeight = clamp(Number(tier.textFontWeight ?? next.tiers[i].textFontWeight), 0, 2);
                    next.tiers[i].numberFontWeight = clamp(Number(tier.numberFontWeight ?? next.tiers[i].numberFontWeight), 0, 2);
                    next.tiers[i].textAllCaps = Boolean(tier.textAllCaps);
                    next.tiers[i].numberAllCaps = Boolean(tier.numberAllCaps);
                }
            }
            return next;
        };

        bridge = {
            updateComboMeter: (payload: string) => {
                if (!payload) return;
                const parts = payload.split('|');
                if (parts.length < 2) return;
                if (!config.enabled) {
                    clearCombo();
                    return;
                }

                actualHitValue = parseInt(parts[0], 10) || 0;
                actualTier = clamp(parseInt(parts[1], 10) || 0, 0, ranks.length - 1);
                actualPoints = parseInt(parts[2] ?? '0', 10) || 0;
                actualPointsRequired = Math.max(1, parseInt(parts[3] ?? '100', 10) || 100);
                actualTotalPoints = parseInt(parts[4] ?? `${actualPoints}`, 10) || actualPoints;

                renderEditPreview();
            },
            showComboMessage: (payload: string) => {
                if (!config.enabled || !config.showFloatingMessages) return;
                if (!payload) return;
                const separator = payload.lastIndexOf('|');
                if (separator <= 0) return;
                const label = payload.slice(0, separator);
                const delta = parseInt(payload.slice(separator + 1), 10) || 0;
                spawnMessage(label, delta);
            },
            setComboTimerPaused: (payload: string) => {
                isTimerPaused = payload === 'true';
                lastFrameTime = performance.now();
                if (!isTimerPaused) requestFrame();
            },
            updateComboUiSettings: (payload: string) => {
                if (!payload) return;
                try {
                    config = parseConfig(payload);
                    styleDirty = true;
                    positionHud();
                    if (!config.enabled) {
                        clearCombo();
                    } else if (config.editMode || hasActiveCombo()) {
                        renderEditPreview();
                    } else {
                        clearCombo();
                    }
                } catch {
                    return;
                }
            },
        };

        preloadAllAssets();
        setHudVisible(false);
        positionHud();

        if (pendingSettingsPayload) {
            bridge.updateComboUiSettings(pendingSettingsPayload);
            pendingSettingsPayload = null;
        }
        if (pendingComboPayload) {
            bridge.updateComboMeter(pendingComboPayload);
            pendingComboPayload = null;
        }
        while (pendingMessages.length > 0) {
            bridge.showComboMessage(pendingMessages.shift() ?? '');
        }

        const handleResize = () => {
            positionHud();
        };

        window.addEventListener('resize', handleResize);

        onCleanup(() => {
            window.removeEventListener('resize', handleResize);
            if (animationFrameId !== 0) {
                cancelAnimationFrame(animationFrameId);
            }
            bridge = null;
            hudRoot.remove();
        });
    });

    return (
        <div
            ref={containerRef}
            style={{
                position: 'absolute',
                top: 0,
                left: 0,
                width: '100vw',
                height: '100vh',
                'pointer-events': 'none',
                overflow: 'hidden',
            }}
        />
    );
}

export default App;
