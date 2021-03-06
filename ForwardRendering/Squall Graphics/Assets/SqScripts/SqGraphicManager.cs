﻿using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityStandardAssets.Utility;

/// <summary>
/// squall graphic manager
/// </summary>
public class SqGraphicManager : MonoBehaviour
{
    [DllImport("SquallGraphics")]
    static extern bool InitializeSqGraphic(int _numOfThreads, int _width, int _height);

    [DllImport("SquallGraphics")]
    static extern void ReleaseSqGraphic();

    [DllImport("SquallGraphics")]
    static extern void UpdateSqGraphic();

    [DllImport("SquallGraphics")]
    static extern void RenderSqGraphic();

    [DllImport("SquallGraphics")]
    static extern int GetRenderThreadCount();

    [DllImport("SquallGraphics")]
    static extern IntPtr GetCpuProfile();

    [DllImport("SquallGraphics")]
    static extern IntPtr GetGpuProfile();

    /// <summary>
    /// number of render threads
    /// </summary>
    [Range(2, 16)]
    public int numOfRenderThreads = 4;

    /// <summary>
    /// global aniso level
    /// </summary>
    [Range(1, 16)]
    public int globalAnisoLevel = 8;

    /// <summary>
    /// reseting frame
    /// </summary>
    [HideInInspector]
    public bool resetingFrame = false;

    /// <summary>
    /// instance
    /// </summary>
    public static SqGraphicManager Instance { private set; get; }

    bool displayFrameData = false;

    void Awake()
    {
        if (Instance != null)
        {
            Debug.LogError("Only one SqGraphicManager can be created.");
            enabled = false;
            return;
        }

        if (numOfRenderThreads < 2)
        {
            numOfRenderThreads = 2;
        }

        if (InitializeSqGraphic(numOfRenderThreads, Screen.width, Screen.height))
        {
            Debug.Log("[SqGraphicManager] Squall Graphics initialized.");
            Instance = this;
        }
        else
        {
            Debug.LogError("[SqGraphicManager] Squall Graphics terminated.");
            enabled = false;
            return;
        }

        Debug.Log("[SqGraphicManager] Number of render threads: " + GetRenderThreadCount());
    }

    void OnApplicationQuit()
    {
        ReleaseSqGraphic();
    }

    void LateUpdate()
    {
        if (resetingFrame)
        {
            resetingFrame = false;
            return;
        }

        if (Input.GetKeyUp(KeyCode.R))
            displayFrameData = !displayFrameData;

        UpdateSqGraphic();
        RenderSqGraphic();
    }

    void OnGUI()
    {
        if (displayFrameData)
        {
            string cpuProfile = Marshal.PtrToStringAnsi(GetCpuProfile());
            string gpuProfile = Marshal.PtrToStringAnsi(GetGpuProfile()) + "\nFPS: " + GetComponent<FPSCounter>().m_CurrentFps;

            GUIStyle fontStyle = new GUIStyle(GUI.skin.textArea);
            fontStyle.normal.textColor = Color.yellow;
            fontStyle.fontSize = 18 * Screen.width / 1920;

            GUI.TextArea(new Rect(0, 0, 400 * Screen.width / 1920, 430 * Screen.height / 1080), cpuProfile, fontStyle);
            GUI.TextArea(new Rect(400 * Screen.width / 1920, 0, 400 * Screen.width / 1920, 430 * Screen.height / 1080), gpuProfile, fontStyle);
        }
    }
}
